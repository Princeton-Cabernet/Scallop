
import Mustache from 'mustache';

export class UIComponent<DataType> {

    protected _container: HTMLDivElement;
    protected _template: string;

    constructor(containerSelector: string, templateSelector: string) {

        if (document.querySelector(containerSelector) != null) {
            this._container = document.querySelector(containerSelector)!;
        } else {
            throw Error(`UIComponent: could not find container ${containerSelector}`);
        }

        if (document.querySelector(templateSelector) != null) {
            this._template = document.querySelector(templateSelector)!.innerHTML.trim();
        } else {
            throw Error(`UIComponent: could not find template ${templateSelector}`);
        }
    }

    public update(data: DataType): void {
        this._container.innerHTML = Mustache.render(this._template, data);
    }
}
