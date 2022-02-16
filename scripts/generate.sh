
cp ./src/dictionaries/OD.h ./src/definitions/Base.h
node scripts/overlay.js ./src/definitions/Base_F1.c ./src/dictionaries/OD.c ./src/dictionaries/Base_F1.c 
node scripts/overlay.js ./src/definitions/Base_F4.c ./src/dictionaries/OD.c ./src/dictionaries/Base_F4.c 

node --experimental-modules scripts/types.js ./src/dictionaries/Base.xdd ./src/dictionaries/OD.h
node scripts/enums.js --experimental-modules