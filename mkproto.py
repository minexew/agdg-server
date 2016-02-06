class Message:
	def __init__(self, id, data=[]):
		self.id = id
		self.data = data

class Field:
	def __init__(self, name, repeated, cpp_type=None, cpp_decode_func='Read', cpp_encode_func='Write',
				ts_decode_func=None, ts_encode_func=None, ts_type=None):
		self.name = name
		self.repeated = repeated
		self.cpp_type = cpp_type
		self.cpp_decode_func = cpp_decode_func
		self.cpp_encode_func = cpp_encode_func
		self.ts_decode_func = ts_decode_func
		self.ts_encode_func = ts_encode_func
		self.ts_type = ts_type if not repeated else 'Array<%s>' % ts_type

	def write_cpp_decl(self, out):
		if not self.cpp_type: raise Exception('undefined cpp_type for %s' % self.name)

		type = self.cpp_type if not self.repeated else 'std::vector<%s>' % self.cpp_type

		print('    %s\t%s;' % (type, self.name), file=out)

	def write_cpp_read(self, out, rename=None):
		if self.repeated:
			print('    uint32_t %s_count;' % self.name, file=out)
			print('    if (!Read(buffer, length, %s_count)) return false;' % self.name, file=out)
			print(file=out)
			print('    for (size_t i = 0; i < %s_count; i++) {' % self.name, file=out)
			print('        %s.emplace_back();' % self.name, file=out)
			self.write_cpp_decode(out, rename='%s.back()' % self.name)
			print('    }', file=out)
			print(file=out)
		else:
			self.write_cpp_decode(out, rename=rename)

	def write_cpp_write(self, out, rename=None):
		if self.repeated:
			print('    if (!Write<uint32_t>(out, %s.size())) return false;' % self.name, file=out)
			print(file=out)
			print('    for (size_t i = 0; i < %s.size(); i++) {' % self.name, file=out)
			self.write_cpp_encode(out, rename='%s[i]' % self.name)
			print('    }', file=out)
			print(file=out)
		else:
			self.write_cpp_encode(out, rename=rename)

	def write_cpp_decode(self, out, rename=None):
		if not self.cpp_decode_func: raise Exception('undefined cpp_decode_func for %s' % self.name)

		print('    if (!%s(buffer, length, %s)) return false;' % (self.cpp_decode_func, rename or self.name), file=out)

	def write_cpp_encode(self, out, rename=None):
		if not self.cpp_encode_func: raise Exception('undefined cpp_encode_func for %s' % self.name)

		print('    if (!%s(out, %s)) return false;' % (self.cpp_encode_func, rename or self.name), file=out)

	def write_ts_read(self, out, rename=None):
		if self.repeated:
			print('    var %s_count = dv.getUint32(offset, true);' % self.name, file=out)
			print('    offset += 4;', file=out)
			print('    %s = [];' % (rename or self.name), file=out)
			print(file=out)
			print('    for (var i = 0; i < %s_count; i++) {' % self.name, file=out)
			print('        var the_%s: any;' % self.name, file=out)
			self.write_ts_decode(out, rename='the_%s' % self.name)
			print('        %s.push(the_%s);' % (rename or self.name, self.name), file=out)
			print('    }', file=out)
			print(file=out)
		else:
			self.write_ts_decode(out, rename=rename)

	def write_ts_write(self, out, rename=None):
		if self.repeated:
			print('    this.dv.setUint32(offset, %s.length, true);' % self.name, file=out)
			print('    offset += 4;', file=out)
			print(file=out)
			print('    for (var i = 0; i < %s.length; i++) {' % self.name, file=out)
			self.write_ts_encode(out, rename='%s[i]' % self.name)
			print('    }', file=out)
			print(file=out)
		else:
			self.write_ts_encode(out, rename=rename)

	def write_ts_decode(self, out, rename=None):
		if not self.ts_decode_func: raise Exception('undefined ts_decode_func for %s' % self.name)

		print('    %s = dv.%s(offset, true);' % (rename or self.name, self.ts_decode_func), file=out)
		print('    offset += 4;', file=out)

	def write_ts_encode(self, out, rename=None):
		if not self.ts_encode_func: raise Exception('undefined ts_encode_func for %s' % self.name)

		print('    this.dv.%s(offset, %s, true);' % (self.ts_encode_func, rename or self.name), file=out)
		print('    offset += 4;', file=out)

class CompoundField(Field):
	def __init__(self, name, typename, data, repeated=False):
		super().__init__(name, repeated, cpp_type=typename, ts_type='any')
		self.typename = typename
		self.data = data

	def write_cpp_typedecl(self, out):
		print('    struct %s {' % self.typename, file=out)

		for field in self.data:
			if callable(getattr(field, 'write_cpp_typedecl', None)): field.write_cpp_typedecl(out)

		for field in self.data:
			field.write_cpp_decl(out)

		print('    };', file=out)

	def write_cpp_decode(self, out, rename=None):
		for field in self.data:
			field.write_cpp_read(out, rename=(rename or self.name) + '.' + field.name)

	def write_cpp_encode(self, out, rename=None):
		for field in self.data:
			field.write_cpp_write(out, rename=(rename or self.name) + '.' + field.name)

	def write_ts_decode(self, out, rename=None):
		if rename:
			print('    %s = {};' % rename, file=out)

		for field in self.data:
			field.write_ts_read(out, rename=(rename or self.name) + '.' + field.name)

	def write_ts_encode(self, out, rename=None):
		for field in self.data:
			field.write_ts_write(out, rename=(rename or self.name) + '.' + field.name)

class HashField(Field):
	def __init__(self, name, repeated=False):
		super().__init__( name, repeated, cpp_type='SHA3_224', ts_type='string')

	def write_ts_decode(self, out, rename=None):
		print('    %s = this.decodeHash(dv, offset);' % (rename or self.name), file=out)
		print('    offset += 28;', file=out)

	def write_ts_encode(self, out, rename=None):
		print('    offset += this.encodeHash(offset, %s);' % (rename or self.name), file=out)

class StringField(Field):
	def __init__(self, name, repeated=False):
		super().__init__(name, repeated, cpp_type='std::string', ts_decode_func='decodeString', ts_type='string')

	def write_ts_decode(self, out, rename=None):
		name = (rename or self.name)

		# FIXME: still wrong
		print('    %s = this.decodeString(dv, offset);' % name, file=out)
		print('    offset += 4 + %s.length;' % name, file=out)

	def write_ts_encode(self, out, rename=None):
		print('    offset += this.encodeString(offset, %s);' % (rename or self.name), file=out)

class Int32Field(Field):
	def __init__(self, name, repeated=False):
		super().__init__(name, repeated, cpp_type='int32_t', ts_type='number',
				ts_decode_func='getInt32', ts_encode_func='setInt32')

class Uint32Field(Field):
	def __init__(self, name, repeated=False):
		super().__init__(name, repeated, cpp_type='uint32_t', ts_type='number',
				ts_decode_func='getUint32', ts_encode_func='setUint32')

class Vec3Field(Field):
	def __init__(self, name, repeated=False):
		super().__init__(name, repeated, cpp_type='glm::vec3', ts_type='pc.Vec3')

	def write_ts_decode(self, out, rename=None):
		print('    %s = new pc.Vec3(dv.getFloat32(offset, true), dv.getFloat32(offset + 4, true), dv.getFloat32(offset + 8, true));'
				% (rename or self.name), file=out)
		print('    offset += 12;', file=out)

	def write_ts_encode(self, out, rename=None):
		name = (rename or self.name)

		print('    this.dv.setFloat32(offset, %s.x, true);' % name, file=out)
		print('    this.dv.setFloat32(offset + 4, %s.y, true);' % name, file=out)
		print('    this.dv.setFloat32(offset + 8, %s.z, true);' % name, file=out)
		print('    offset += 12;', file=out)


# CLIENT -> SERVER

messages = {}

messages['CHello'] = Message(
	id=1,
	data=[
		HashField('token')
	]
)

messages['CEnterWorld'] = Message(
	id=2,
	data=[
		StringField('characterName')
	]
)

messages['CZoneLoaded'] = Message(
	id=3,
)

messages['CPlayerMovement'] = Message(
	id=4,
	data=[
		Vec3Field('pos'),
		Vec3Field('dir'),
		Vec3Field('velocity')
	]
)

messages['CPong'] = Message(
	id=5,
)

messages['CChatSay'] = Message(
	id=30,
	data=[
		StringField('text'),
	],
)

# SERVER -> CLIENT

messages['SHello'] = Message(
	id=1,
	data=[
		StringField('characters', repeated=True)
	]
)

messages['SLoadZone'] = Message(
	id=2,
	data=[
		StringField('zoneName'),
		HashField('zoneRef'),
	]
)

messages['SZoneState'] = Message(
	id=3,
	data=[
		Int32Field('playerEid'),
		StringField('playerName'),
		Vec3Field('playerPos'),
		Vec3Field('playerDir'),
		CompoundField('entities', 'Entity', repeated=True, data=[
			Uint32Field('eid'),
			Uint32Field('flags'),
			StringField('name'),
			Vec3Field('pos'),
			Vec3Field('dir')
		])
	]
)

messages['SPing'] = Message(
	id=4,
)

messages['SEntitySpawn'] = Message(
	id=20,
	data=[
		CompoundField('entity', 'Entity', data=[
			Uint32Field('eid'),
			Uint32Field('flags'),
			StringField('name'),
			Vec3Field('pos'),
			Vec3Field('dir')
		])
	]
)

messages['SEntityDespawn'] = Message(
	id=21,
	data=[
		Uint32Field('eid'),
	],
)

messages['SEntityUpdate'] = Message(
	id=22,
	data=[
		Uint32Field('eid'),
		Vec3Field('pos'),
		Vec3Field('dir'),
		Vec3Field('velocity')
	]
)

messages['SChatSay'] = Message(
	id=30,
	data=[
		Uint32Field('eid'),
		StringField('text'),
	],
)

# sort messages by ID, first C->S, then S->C
sorted_messages = sorted(messages.items(), key=lambda tuple: tuple[1].id + (1000 if tuple[0][0] == 'S' else 0))

def generate_cpp_header():
	with open('src/realm/protocol_generated.hpp', 'wt') as out:
		for name, msg in sorted_messages:
			print('enum { k%s = %d };' % (name, msg.id), file=out)

		print(file=out)

		for name, msg in sorted_messages:
			print('struct %s {' % name, file=out)

			for field in msg.data:
				if callable(getattr(field, 'write_cpp_typedecl', None)): field.write_cpp_typedecl(out)

			for field in msg.data:
				field.write_cpp_decl(out)

			if len(msg.data): print(file=out)
			print('    bool Decode(const uint8_t* buffer, size_t length);', file=out)
			print('    bool Encode(std::ostream& out) const;', file=out)

			print('};', file=out)

def generate_cpp_source():
	with open('src/realm/protocol_generated_impl.hpp', 'wt') as out:
		for name, msg in sorted_messages:
			print('bool %s::Decode(const uint8_t* buffer, size_t length) {' % name, file=out)

			for field in msg.data:
				field.write_cpp_read(out)

			print('    return true;', file=out)
			print('}', file=out)
			print(file=out)
			print('bool %s::Encode(std::ostream& out) const {' % name, file=out)
			print('    Begin(out, k%s);' % name, file=out)

			for field in msg.data:
				field.write_cpp_write(out)

			print('    return true;', file=out)
			print('}', file=out)
			print(file=out)

def generate_ts_source():
	with open('../agdg-client/src/agdg-client/realm-protocol-generated.ts', 'wt') as out:
		print('/// <reference path="realmprotocol.ts"/>', file=out)
		print(file=out)
		print('module RealmProtocol {', file=out)
		print('export class RealmProtocol extends RealmProtocolBase {', file=out)

		for name, msg in sorted_messages:
			print('decode%s(dv: DataView) {' % name, file=out)
			print('    var offset = 1;', file=out)
			print('    var data: any = {};', file=out)

			for field in msg.data:
				field.write_ts_read(out, rename='data.' + field.name)

			print('    return data;', file=out)
			print('}', file=out)
			print(file=out)
			arguments = ', '.join(['%s: %s' % (field.name, field.ts_type) for field in msg.data])
			print('send%s(%s) {' % (name, arguments), file=out)
			print('    var offset = 1;', file=out)
			print('    this.dv.setUint8(0, %d);' % msg.id, file=out)

			for field in msg.data:
				field.write_ts_write(out)

			print('    this.ws.send(this.ab.slice(0, offset));', file=out)
			print('};', file=out)
			print(file=out)

		print('}', file=out)
		print('}', file=out)

generate_cpp_header()
generate_cpp_source()
generate_ts_source()
