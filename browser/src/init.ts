import {Handler, WorkerDescription, WorkResult} from "./server";
import {registerWorker, Worker} from "./PluginAPI";

export enum SelectMode{
    ID,
    CLASS,
    TAG,
    TAGNAME
}

export namespace SelectMode {
    export function fromName(name:string) : SelectMode{
        switch(name){
            case 'ID': return SelectMode.ID;
            case 'CLASS': return SelectMode.CLASS;
            case 'TAG': return SelectMode.TAG;
            case 'TAGNAME': return SelectMode.TAGNAME;
            default: throw new Error(`Unknown SelectMode of ${name}`);
        }
    }
}

export enum BrowseDataMode{
    DOM,
    HTTP_REQUEST,
    NODATA,
    OTHER
}

export namespace BrowseDataMode {
    export function fromName(name: string): BrowseDataMode{
        switch(name){
            case 'DOM': return BrowseDataMode.DOM;
            case 'HTTP_REQUEST': return BrowseDataMode.HTTP_REQUEST;
            case 'NODATA': return BrowseDataMode.NODATA;
            case 'OTHER': return BrowseDataMode.OTHER;
            default: throw new Error(`Unknown BrowseDataMode of ${name}`);
        }
    }
}

export interface ElementSelectorJson{
    mode: string;
    param: string;
    index: number;
}

export class ElementSelector {
    public mode: SelectMode;
    public param: string;
    public index: number;
    public constructor(json: ElementSelectorJson) {
        this.mode = SelectMode.fromName(json.mode);
        this.param = json.param;
        this.index = json.index;
    }

    public toCSS(): string{
        switch (this.mode) {
            case SelectMode.ID:
                return `#${this.param}`;
            case SelectMode.CLASS:
                return `.${this.param.trim().split(/\s+/).join(".")}`;
            case SelectMode.TAG:
            case SelectMode.TAGNAME:
                return this.param;
        }
    }
}

class UrlAction extends Worker {
    public readonly url : string;
    public constructor(info : unknown) {
        super(info);
        type correct = {
            url: string;
        }
        this.url = (info as correct).url;
    }

    public async work(handler: Handler): Promise<WorkResult> {
        handler.page = await handler.newPage();
        const startedAt = Date.now();
        handler.logger.info({url: this.url}, "打开页面开始");
        await handler.page.goto(this.url,{
            waitUntil: 'domcontentloaded'
        });
        handler.logger.info({url: handler.page.url(), durationMs: Date.now() - startedAt}, "打开页面完成");
        return {};
    }

}

class ClickAction extends Worker {
    public readonly selector: ElementSelector;
    public constructor(info : unknown) {
        super(info);
        this.selector = new ElementSelector(info as ElementSelectorJson);
    }

    public async work(handler: Handler): Promise<WorkResult> {
        const css = this.selector.toCSS();
        const startedAt = Date.now();
        handler.logger.info({selector: css, index: this.selector.index}, "点击元素开始");
        try {
            await handler.page.locator(css).nth(this.selector.index).click();
            handler.logger.info({selector: css, index: this.selector.index, durationMs: Date.now() - startedAt}, "点击元素完成");
            return {};
        } catch (error) {
            const reason = error instanceof Error ? error.message : String(error);
            handler.logger.warn({selector: css, index: this.selector.index, error: reason}, "点击元素失败");
            throw new Error(`click failed for selector '${css}' at index ${this.selector.index}: ${reason}`);
        }
    }
}

class CrawlAction extends Worker {
    public readonly mode: BrowseDataMode;
    public readonly description: string;
    public constructor(info : unknown) {
        super(info);
        type correct = {
            data_mode: string;
            target: string;
        };
        const data = info as correct;
        this.mode = BrowseDataMode.fromName(data.data_mode);
        this.description = data.target;
    }

    public async work(handler: Handler): Promise<WorkResult | WorkResult[]> {
        interface back extends WorkResult {
            data: string | object;
            clean_data?: string;
        }
        const returner = (data: string) => {
            try{
                return {
                    data: JSON.parse(data)
                }
            }catch(e: any) {
                return {
                    data
                }
            }
        };
        switch (this.mode) {
            case BrowseDataMode.DOM: {
                try{
                    const json = JSON.parse(this.description) as ElementSelectorJson;
                    const selector = new ElementSelector(json);
                    const css = selector.toCSS();
                    handler.logger.info({selector: css, index: selector.index}, "读取 DOM 数据开始");
                    const target = handler.page.locator(css).nth(selector.index);
                    const text = await target.textContent();
                    handler.logger.info({selector: css, index: selector.index, textLength: (text ?? "").length}, "读取 DOM 数据完成");
                    return {
                        data: returner(text ?? await target.innerText()),
                        clean_data: (text ?? "").replace(/s+/g,"  ").trim()
                    } as back
                }catch(e){
                    throw new Error(`错误的DOM元素描述，得到的：${this.description}`)
                }
            }
            case BrowseDataMode.HTTP_REQUEST: {
                handler.logger.info({target: this.description, recordedResponseCount: handler.records.size}, "匹配请求数据开始");
                const backs = [...handler.records.entries()]
                    .filter(([key,]) => !key.handled && key.url.includes(this.description))
                    .map(([key,record]) => {
                        key.handled = true;
                        return record;
                    });
                if(backs.length <= 0) {
                    handler.logger.warn({target: this.description, recordedResponseCount: handler.records.size}, "请求数据为空，没有匹配到合格响应");
                    await handler.collectDebugArtifacts(this.description);
                    return {};
                }
                handler.logger.info({target: this.description, dataCount: backs.length}, "匹配请求数据完成");
                if(backs.length == 1){
                    return returner(backs[0].body);
                }else return backs.map(record => returner(record.body));
            }
            case BrowseDataMode.NODATA: return {}
            case BrowseDataMode.OTHER: throw new Error('Bad settings of worker, get OTHER but no registered handler')
        }
    }
}

class DoWhileAction extends Worker {
    public readonly failOrSucceed: boolean;
    public readonly maxCount: number;
    public readonly workers: Worker[];
    public constructor(info : unknown) {
        super(info);
        type correct = {
            failOrSucceeded: boolean;
            maxCount: number;
            workers: WorkerDescription[];
        };
        const data = info as correct;
        this.failOrSucceed = data.failOrSucceeded;
        this.maxCount = data.maxCount;
        this.workers = data.workers.map(worker => Worker.fromDescription(worker));
    }

    public async work(handler: Handler): Promise<WorkResult | WorkResult[]> {
        let count: number = 0;
        let failed: boolean = false;
        type backFormat = (WorkResult | WorkResult[])[];
        const backs: backFormat[] = [];
        do{
            handler.logger.info({iteration: count + 1, maxCount: this.maxCount}, "循环工作项开始");
            let result: backFormat = [];
            for (const worker of this.workers) {
                try{
                    result.push(await worker.work(handler));
                    failed = false;
                }catch(e){
                    handler.logger.warn({iteration: count + 1, error: e}, "循环内工作项失败，继续判断循环条件");
                    failed = true;
                }
            }
            backs.push(result);
            count++;
        }while (count < this.maxCount && failed == this.failOrSucceed)
        return backs;
    }
}

export default function serverInit(){
    registerWorker("UrlAction",{
        getWorker: info => new UrlAction(info)
    });
    registerWorker("ClickAction",{
        getWorker: info => new ClickAction(info)
    });
    registerWorker("CrawlAction",{
        getWorker: info => new CrawlAction(info)
    });
    registerWorker("DoWhileAction",{
        getWorker: info => new DoWhileAction(info)
    });
}
