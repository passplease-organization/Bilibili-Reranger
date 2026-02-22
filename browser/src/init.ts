import {Handler, registerWorker, Worker, WorkResult} from "./server";

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
            default: throw new Error('Unknown SelectMode');
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
        handler.page = await handler.browse.newPage()
        await handler.page.goto(this.url);
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
        const sel = this.selector;
        let css = "";
        switch (sel.mode) {
            case SelectMode.ID:
                css = `#${sel.param}`;
                break;
            case SelectMode.CLASS:
                css = `.${sel.param}`;
                break;
            case SelectMode.TAG:
            case SelectMode.TAGNAME:
                css = sel.param;
                break;
            default:
                throw new Error("Unknown SelectMode");
        }
        await handler.page.locator(css).nth(sel.index).click();
        return {};
    }
}

export default function serverInit(){
    registerWorker("UrlAction",{
        getWorker: info => new UrlAction(info)
    })
}