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
            default: throw new Error(`Unknown SelectMode of ${name}`);
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
                return `.${this.param}`;
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
        await handler.page.goto(this.url,{
            waitUntil: 'domcontentloaded'
        });
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
        await handler.page.locator(this.selector.toCSS()).nth(this.selector.index).click();
        return {};
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
            data: string;
            clean_data?: string;
        }
        switch (this.mode) {
            case BrowseDataMode.DOM: {
                try{
                    const json = JSON.parse(this.description) as ElementSelectorJson;
                    const selector = new ElementSelector(json);
                    const target = handler.page.locator(selector.toCSS()).nth(selector.index);
                    const text = await target.textContent();
                    return {
                        data: text ?? await target.innerText(),
                        clean_data: (text ?? "").replace(/s+/g,"  ").trim()
                    } as back
                }catch(e){
                    throw new Error(`错误的DOM元素描述，得到的：${this.description}`)
                }
            }
            case BrowseDataMode.HTTP_REQUEST: {
                const backs = [...handler.records.values()].filter(response => response.url.includes(this.description));
                if(backs.length == 1){
                    return {
                        data: backs[0].body
                    }
                }else return backs.map(record => ({data: record.body} as back))
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
            const result: backFormat = [];
            for (const worker of this.workers) {
                try{
                    result.push(await worker.work(handler));
                    failed = false;
                }catch(e){
                    failed = true;
                }
            }
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