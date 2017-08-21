class Dialogues {
    constructor(entity, dialogues) {
        this.entity = entity;
        this.dialogues = dialogues;
        this.states = {};

        this.triggers = {};

        for (var node of dialogues) {
            if (!node.triggeredBy)
                continue;

            for (var t of node.triggeredBy)
                this.triggers[t] = node;
        }
    }

    enterNode(state, entity, node) {
        state.busy = true;

        setTimeout(() => {
            state.busy = false;

            if (node.message)
                this.entity.say(node.message, true);

            if (node.startEvents) {
                for (var event of node.startEvents)
                    this.startEvent(event, entity);
            }

            if (node.options) {
                for (var option of node.options)
                    entity.onEntityDidSay(this.entity, `<a class="dlg-reply">${option.option}</a>`, true);

                state.node = node;
            }
            else
                state.node = null;
        }, 0.5);
    }

    static fromFile(entity, path) {
        var parser = new DialogueParser();
        return new Dialogues(entity, parser.parseFile(path + '.dialogue'));
    }

    onChat(entity, text) {
        var state = this.states[entity.eid];

        if (state && state.node) {
            if (state.busy)
                return;

            if (state.node.options) {
                for (var option of state.node.options) {
                    if (text == option.option) {
                        this.enterNode(state, entity, option);
                        return true;
                    }
                }
            }

            return false;
        }
        else {
            var startNode = this.triggers[text.toLowerCase()];

            if (startNode) {
                if (!state) {
                    state = {};
                    this.states[entity.eid] = state;
                }

                this.enterNode(state, entity, startNode);
                return true;
            }
            else
                return false;
        }
    }

    startEvent(event, entity) {
        g_log.warning(`Dialogues.startEvent('${event}') called (entity '${this.entity.name}'), but not overriden`);
    }
}

class DialogueParser {
    parseDialogue(indent, header) {
        var node = this.parseNode(indent);
        node.name = header[1];
        return node;
    }

    parseFile(filename) {
        print('parsing', filename);

        var file = realm.loadFileAsString(filename);
        this.lines = file.split('\n').map(s => s.replace('\r', ''));
        this.lineNo = 0;

        var indent = 0;
        var dialogues = [];

        try {
            while (true) {
                var tokens = this.parseLine(indent);

                if (!tokens)
                    break;
                else if (tokens[0] == 'dialogue')
                    dialogues.push(this.parseDialogue(indent + 1, tokens));
                else
                    throw 'expected "dialogue"';
            }
        }
        catch (ex) {
            throw `${filename}:${this.lastLineNo + 1}:${ex}`;
        }

        return dialogues;
    }

    parseLine(expectedIndent) {
        while (this.lineNo < this.lines.length) {
            var line = this.lines[this.lineNo];

            // determine indent
            for (var i = 0; i < line.length; i++) {
                if (line[i] == '\t')
                    throw 'tab found in indentation - use multiple of 4 spaces';

                if (line[i] != ' ')
                    break;
            }

            if (i == line.length || line[i] == '#') {
                this.lineNo++;
                continue;
            }

            if (i % 4 != 0)
                throw 'invalid indentation - use multiple of 4 spaces';

            const indent = i / 4;

            if (indent < expectedIndent)
                return null;
            else if (indent > expectedIndent)
                throw 'unexpected indent ' + i + ' - expected ' + expectedIndent + ' or less';

            if (line[i] == '-' || line[i] == '>') {
                this.lastLineNo = this.lineNo++;
                return [line[i], line.substr(i + 2)];
            }
            else {
                this.lastLineNo = this.lineNo++;
                return line.substr(i).split(' ');
            }
        }

        this.lastLineNo = undefined;
        return null;
    }

    parseNode(indent) {
        var node = {};

        while (true) {
            var tokens = this.parseLine(indent);

            if (!tokens)
                break;

            if (tokens[0] == '-') {
                if (node.message !== undefined)
                    throw 'message already set!';

                node.message = tokens[1];
            }
            else if (tokens[0] == '>') {
                node.options = node.options || [];

                var reply = this.parseNode(indent + 1);
                reply.option = tokens[1];
                node.options.push(reply);
            }
            else if (tokens[0].toLowerCase() == 'startevent') {
                node.startEvents = node.startEvents || [];

                node.startEvents.push(tokens[1]);
            }
            else if (tokens[0].toLowerCase() == 'triggeredby') {
                if (node.triggeredBy !== undefined)
                    throw 'triggeredBy already set!';

                node.triggeredBy = [];

                while (true) {
                    var tokens = this.parseLine(indent + 1);

                    if (!tokens)
                        break;

                    if (tokens[0] != '-')
                        throw 'expected "- <trigger message>"';

                    node.triggeredBy.push(tokens[1].toLowerCase());
                }
            }
            else
                throw 'unexpected ' + tokens[0];
        }

        return node;
    }
}
