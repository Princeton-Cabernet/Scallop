
import { ClientController } from './ClientController'
import { Util } from './Util';

new ClientController(
    Util.getScriptDataAttribute("config")
);
