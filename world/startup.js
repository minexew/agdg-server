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
    if (typeof(obj) == "object" && (obj.join == undefined)) {
        string.push("{");
        var first = true;
        for (var prop in obj) {
            if (!first) string.push(", "); else first = false;
            string.push(prop, ": ", convertToText(obj[prop]));
        };
        string.push("}");

    //is array
    } else if (typeof(obj) == "object" && !(obj.join == undefined)) {
        string.push("[")
        for(var prop in obj) {
            string.push(convertToText(obj[prop]), ",");
        }
        string.push("]")

    //is function
    } else if (typeof(obj) == "function") {
        string.push(obj.toString())

    //all other values can be done with JSON.stringify
    } else {
        string.push(JSON.stringify(obj))
    }

    return string.join("")
}

function print(...args) {
    log_string(args.map(function(value) {
        if (value === undefined)
            return 'undefined';
        else if (typeof(value) == 'object')
            return convertToText(value)
        else
            return value.toString()
    }).join(' '))
}

function areClose(a, b) {
    var a = a.pos;
    var b = b.pos;

    var dx = a[0] - b[0];
    var dy = a[1] - b[1];
    var dz = a[2] - b[2];

    return (dx * dx + dy * dy + dz * dz < 10 * 10);
}

class BaseAIEntity {
    constructor(zoneInstance, entity) {
        this.zoneInstance = zoneInstance;
        this.entity = entity;

        zoneInstance.onChat((entity, text, html) => {
            if (entity && areClose(this.entity, entity))
                this.onChat(entity, text, html);

            return true;
        });

        //zoneInstance.onEntityDespawn
    }

    onChat(entity, text, html) {
    }
}
class Abalath extends BaseAIEntity {
    onChat(entity, text, html) {
        this.zoneInstance.broadcastChat(this.entity, 'I can hear you, ' + entity.name + '!', false);
    }
}

realm.onRealmInit(() => {
	print('Scripting engine engaged')
});

realm.onZoneInstanceCreate(instance => {
	print('onZoneInstanceCreate', instance.id)

    entity = instance.spawnTestEntity("Abalath", [4, 4, 0.5]);
    new Abalath(instance, entity);

	instance.onPlayerHasEntered(player => {
		//print('onPlayerEnter', player)
		//instance.broadcastChat(null, `Hi, <i>${player.name}</i>!`, true)
	})

    instance.onChat((entity, message, html) => {
        if (message.indexOf('/exec ') === 0) {
            eval(message.substr(6))
            return false
        }

        return true
    })
})
