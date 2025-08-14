
export class Util {

    public static time(): string {

        const now = new Date();
        return `${now.getTime()}`;
    }

    public static getScriptDataAttribute(attribute: string): any {

        const scripts = document.getElementsByTagName("script");
        var param = scripts[scripts.length-1].getAttribute("data-" + attribute);
        return param ? param : null;
    }
}
