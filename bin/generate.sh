
cp ./src/dictionaries/OD.h ./src/definitions/Base.h
node bin/overlay.js ./src/definitions/Base_F1.c ./src/dictionaries/OD.c ./src/dictionaries/Base_F1.c 
node bin/overlay.js ./src/definitions/Base_F4.c ./src/dictionaries/OD.c ./src/dictionaries/Base_F4.c 

node --experimental-modules bin/types.js ./src/dictionaries/Base.xdd ./src/dictionaries/OD.h
node bin/enums.js --experimental-modules