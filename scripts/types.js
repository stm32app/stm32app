const fs = require('fs');
const inflection = require('inflection');
const { stringify } = require('querystring');

const underscore = (string) => {
  return inflection.underscore(
    string
      .replace(/(\d)([A-Z])/g, (m, f, rest) => f + rest.toLowerCase())
      .replace(/([A-Z])([A-Z]+)/g, (m, f, rest) => f + rest.toLowerCase())
  )
  .replace(/__/g, '_')
  .replace(/([a-z])_(\d)/g, '$1$2')
}

const xml = fs.readFileSync(process.argv[2]).toString();
const od = fs.readFileSync(process.argv[3]).toString();

const types = {}
const files = {};
const structs = {};
const indecies = {};

const findDefinition = (uid) => {
  const re = new RegExp(`<q1:parameter uniqueID="${uid.toUpperCase()}"[^>]*?>([\\s\\S]*?)</q1:parameter>`, 'g')
  const match = re.exec(xml)
  if (match) {
    const labelRe = new RegExp(`<CANopenObject[^>]*?index="${uid.toUpperCase().replace(/[^\d]/g, '')}"[^>]*?name="([^"]+?)"`, 'g')
    const label = labelRe.exec(xml);
    return {
      description: (match[1].match(/<description[^>]+>([\s\S]*?)<\/description>/) || [null, null])[1],
      label: label ? label[1] : null
    }
  }
  return {}
}

var toHex = function (rgb) { 
  var hex = Number(rgb).toString(16);
  if (hex.length < 2) {
       hex = "0" + hex;
  }
  return hex.toUpperCase();
};

od.replace(/\{\s*([^}]+?)\s*\}[^}]+?x([3-9].*?)_([a-z]+)([A-Z][^_\s,;]+)/g, (match, struct, index, type, name) => {
  struct = struct.replace(/\n\s+/g, '\n    ')
  struct = struct.replace(/( )([^; ]+);/g, (m, w, v) => w + underscore(v) + ';')
  struct = struct.replace(/[^;]+;(?:\n|$)/, ''); // remove highest index entry
  name = underscore(name);


  const definition = findDefinition(`UID_OBJ_${index}`);

  var subtype = 'properties'

  const accessors = [];
  const prototypes = [];
  const defs = [];

  const typeName = type == 'core' ? `${name}_${subtype}` : `${type}_${name}_${subtype}`
  const structName = `${type}_${name}`
  const filePath = `./src/${type}/${name}.h`;
  structs[structName] = (definition.description || '').split('\n')[0];
  if (indecies[structName] == null){
    indecies[structName] = index;
  }

  var foundPhase = false;
  struct.split(/;/).map((pair, attributeIndex) => {
    if (!pair) return;
    const [, dataTypeBasic, attribute, arrayLength] = pair.trim().match(/([^\s]+)\s+([^\[]+)(\[[^]+\])?/)
    const dataType = dataTypeBasic;


    const constant = `${type.toUpperCase()}_${name.toUpperCase()}_${attribute.toUpperCase()}`
    defs.push(`${constant} = 0x${(toHex(attributeIndex + 1))}`);
    const attributeOD = `${index.replace(/\d\d$/, 'XX')}${toHex(attributeIndex + 1)}`
    const attributeDefinition = findDefinition(`UID_SUB_${index}${toHex(attributeIndex + 1)}`);
    const shorttype = dataType.replace(/_t/, '').replace(/([a-z])[a-z]+/, '$1');
    if (attributeDefinition.description) {
      struct = struct.replace(' ' + attribute + ';', ' ' + attribute + '; // ' + attributeDefinition.description.split(/\n/g)[0] + ' ')
    }
    if (attribute == 'phase') foundPhase = true;
    if (foundPhase) {
      
  prototypes.push(
`
ODR_t ${type}_${name}_set_${attribute}(${type}_${name}_t *${name}, ${dataType} value); // 0x${attributeOD}: ${name} ${attribute}
${dataType} ${type}_${name}_get_${attribute}(${type}_${name}_t *${name}); // 0x${attributeOD}: ${name} ${attribute}`)

if (arrayLength) {
//accessors.push(`/* 0x${attributeOD}: ${attributeDefinition.description} */
//#define ${type}_${name}_set_${attribute}(${name}, value, size) actor_set_property${dataType == 'char' ? '_string' : ''}(${name}->actor, ${constant}, value, size)`);
accessors.push(`/* 0x${attributeOD}: ${attributeDefinition.description} */
static inline void ${type}_${name}_set_${attribute}(${type}_${name}_t *${name}, ${dataType} *value, size_t size) {
    actor_set_property${dataType == 'char' ? '_string' : ''}(${name}->actor, ${constant}, value, size);
}`);
} else {
accessors.push(`/* 0x${attributeOD}: ${attributeDefinition.description} */
static inline void ${type}_${name}_set_${attribute}(${type}_${name}_t *${name}, ${dataType} value) { 
    actor_set_property_numeric(${name}->actor, ${constant}, (uint32_t)(value), sizeof(${dataType}));
}`);
//accessors.push(`/* 0x${attributeOD}: ${attributeDefinition.description} */
//#define ${type}_${name}_set_${attribute}(${name}, value) actor_set_property_numeric(${name}->actor, ${constant}, (uint32_t) (value), sizeof(${dataType}))`);
}

/*
if (dataType == 'char') {
  accessors.push(`#define ${type}_${name}_get_${attribute}(${name}) ((${dataType} *) actor_get_property_pointer(${name}->actor, ${constant}))`);
} else {
  accessors.push(`#define ${type}_${name}_get_${attribute}(${name}) *((${dataType} *) actor_get_property_pointer(${name}->actor, ${constant}))`);

} */
if (dataType == 'char') {
accessors.push(`static inline ${dataType} *${type}_${name}_get_${attribute}(${type}_${name}_t *${name}) {
    return (${dataType} *) actor_get_property_pointer(${name}->actor, ${constant});
}`);
} else {
accessors.push(`static inline ${dataType} ${type}_${name}_get_${attribute}(${type}_${name}_t *${name}) {
    return *((${dataType} *) actor_get_property_pointer(${name}->actor, ${constant}));
}`);
} 

if (!'USE MACROS?')    accessors.push(
`/* 0x${attributeOD}: ${name} ${attribute} */
__attribute__((weak)) ODR_t ${type}_${name}_set_${attribute}(${type}_${name}_t *${name}, ${dataType} value) {
  return OD_set_${shorttype}(${name}->actor->${subtype}, ${constant}, value, false);
}
__attribute__((weak)) ${dataType} ${type}_${name}_get_${attribute}(${type}_${name}_t *${name}) {
  ${dataType} value;
  OD_get_${dataType.replace(/_t/, '').replace(/([a-z])[a-z]+/, '$1')}(${name}->actor->${subtype}, ${constant}, &value, false);
  return value; 
}`)
} /**
ODR_t ${name}_read_${attribute}(${type}_${name}_t *${name}, ${dataType} value) {
  return OD_set_${dataType.replace(/_t/, '').replace(/([a-z])[a-z]+/, '$1')}(${name}->actor->${subtype}, ${constant}, value, false);
} */
  }).join('\n')


  try {
      
    if (!types[typeName]) {
      types[typeName] = true;
      if (!files[filePath]) files[filePath] = { types: [], accessors: [], prototypes: [], defs: [] };
      files[filePath].types.push(`/* 0x${index}: ${definition.label}${definition.description && '\n   ' + definition.description} */\ntypedef struct ${typeName} {\n    uint8_t parameter_count;\n${struct}\n} ${typeName}_t;`)
      files[filePath].defs.push(`typedef enum ${typeName}_properties {\n  ${defs.join(',\n  ')}\n} ${typeName}_properties_t;`)
      files[filePath].accessors.push(accessors.join('\n'))
      files[filePath].prototypes.push(prototypes.join(''))
      files[filePath].struct = structName;
    }
  } catch(e) {
    console.log(e.toString());
  }


})


for (var [filePath, value] of Object.entries(files)) {
  try {
    var oldHeader = fs.readFileSync(filePath).toString()
    if (oldHeader) {
      oldHeader = oldHeader.replace(/(\/\*\s*Start of autogenerated OD types\s*\*\/)[\s\S]*(\/\*\s*End of autogenerated OD types\s*\*\/)/, (m, s, e) => {
        return s + `\n${value.types.join('\n\n')}\n`+ e;
      })
      oldHeader = oldHeader.replace(/(\/\*\s*Start of autogenerated OD accessors\s*\*\/)[\s\S]*(\/\*\s*End of autogenerated OD accessors\s*\*\/)/, (m, s, e) => {
        return s + `\n${value.defs.join('\n')}\n`+ `\n${value.accessors.filter(Boolean).join('\n')}\n` + e;
      })
      fs.writeFileSync(filePath, oldHeader);
    }
  } catch(e) {
    console.log(e.toString());
  }
}


const keys = Object.keys(structs).sort((a, b) => {
  return parseInt(indecies[a]) - parseInt(indecies[b])
})

// update types.h with forward definitions of structs
var enums = `enum actor_type {
${keys.map((struct) => {
  return `    ${struct.toUpperCase()} = 0x${indecies[struct]}, /* ${structs[struct]}*/`
}).join('\n')}
};
`
const defs = keys.map((struct) => {
  if (struct == 'core_app') 
    return null
  return `typedef struct ${struct} ${struct}_t /* ${structs[struct]}*/;`
}).filter(Boolean)
fs.writeFileSync('./src/definitions/dictionary.h', `\n${defs.join('\n')}\n\n${enums}\n`);
