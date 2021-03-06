//Make an object a string that evaluates to an equivalent object
//  Note that eval() seems tricky and sometimes you have to do
//  something like eval("a = " + yourString), then use the value
//  of a.
//
//  Also this leaves extra commas after everything, but JavaScript
//  ignores them.
function convertToText(obj) {
    //create an array that will later be joined into a string.
    var string = [];

    //is object
    //    Both arrays and objects seem to return "object"
    //    when typeof(obj) is applied to them. So instead
    //    I am checking to see if they have the property
    //    join, which normal objects don't have but
    //    arrays do.
    if (typeof obj === "object" && obj.join === undefined) {
        string.push("{");
        var first = true;
        for (let prop in obj) {
            if (!first) string.push(", "); else first = false;
            string.push(prop, ": ", convertToText(obj[prop]));
        }
        string.push("}");

    //is array
    } else if (typeof obj === "object" && !(obj.join === undefined)) {
        string.push("[");
        for (let prop in obj) {
            string.push(convertToText(obj[prop]), ",");
        }
        string.push("]");

    //is function
    } else if (typeof obj === "function") {
        string.push(obj.toString());

    //all other values can be done with JSON.stringify
    } else {
        string.push(JSON.stringify(obj));
    }

    return string.join("");
}

function print(...args) {
    log_string(args.map((value) => {
        if (value === undefined)
            return 'undefined';
        else if (typeof value === 'object')
            return convertToText(value);
        else
            return value.toString();
    }).join(' '));
}

function areClose(aEntity, bEntity) {
    const a = aEntity.pos;
    const b = bEntity.pos;

    const dx = a[0] - b[0];
    const dy = a[1] - b[1];
    const dz = a[2] - b[2];

    return dx * dx + dy * dy + dz * dz < 10 * 10;
}

class BaseAIEntity {
    constructor(zoneInstance, entity) {
        this.zoneInstance = zoneInstance;
        this.entity = entity;

        // TODO: replace with this.entity.onEntityDidSay
        zoneInstance.didChat((entity, text, html) => {
            if (this.didChat && entity && entity !== this.entity && areClose(this.entity, entity))
                this.didChat(entity, text, html);

            return true;
        });

        //zoneInstance.onEntityDespawn
    }

}
class Abalath extends BaseAIEntity {
    constructor(zoneInstance, entity) {
        super(zoneInstance, entity);

        this.dialogues = Dialogues.fromFile(entity, 'world/zones/test_zone/abalath');
        this.dialogues.startEvent = (...args) => this.startEvent.apply(this, args);
    }

    didChat(entity, text, html) {
        if (!entity.isPlayer)
            return;

        if (!this.dialogues.onChat(entity, text)) {
            setTimeout(() => this.entity.say(`I have no reaction to that, ${entity.name}...`), 0.5);
        }
    }

    spawnMinion() {
        var x = this.entity.pos[0] - Math.random() * 5;
        var y = this.entity.pos[1] - Math.random() * 5;

        var instance = this.zoneInstance;
        var entity = instance.spawnTestEntity("Abalath's Minion", [x, y, 0.5]);
        new AbalathMinion(instance, entity);
        return entity;
    }

    startEvent(event, entity) {
        if (this.minions) {
            this.entity.say(`I'm currently busy with somebody else ${entity.name}, but I'm totally coming for you!`);
        }
        else {
            this.entity.say(`You will pay for this. My minions shall destroy you!`);
            this.minions = [];

            setTimeout(() => {
                this.entity.say(`Rise, minions!`);

                setTimeout(() => {
                    for (var i = 0; i < 3; i++)
                        this.minions.push(this.spawnMinion());

                    setTimeout(() => {
                        this.minions.forEach(entity => entity.despawn());
                        this.minions = null;

                        this.entity.say(`Well shit.`);
                    }, 7.5);
                }, 1);
            }, 3);
        }
    }
}

class AbalathMinion extends BaseAIEntity {
    constructor(zoneInstance, entity) {
        super(zoneInstance, entity);

        setTimeout(() => {
            var messages = ["I'm scared", "Not gonna fight that guy", "I'm running away!", "Pay us more and we can talk", "Fuck you Abalath"];

            this.entity.say(messages[Math.floor(Math.random() * messages.length)]);
        }, Math.random() * 5);
    }
}

realm.onRealmInit(() => {
});

var timers = [];

realm.onTick(() => {
    for (var i = 0; i < timers.length; i++) {
        if (timers[i][1]-- <= 0) {
            timers[i][0]();
            timers.splice(i, 1);
            i--;
        }
    }
});

function setTimeout(func, timeout) {
    timers.push([func, Math.floor(timeout * 100)]);
}

realm.onZoneInstanceCreate(instance => {
    entity = instance.spawnTestEntity("Abalath", [4, 4, 0.5]);
    new Abalath(instance, entity);

    instance.playerDidEnter(player => {
        //instance.broadcastChat(null, `Hi, <i>${player.name}</i>!`, true)
    });

    instance.willChat((entity, message, html) => {
        if (message.indexOf('/exec ') === 0) {
            eval(message.substr(6));
            return false;
        }

        return true;
    });
});
