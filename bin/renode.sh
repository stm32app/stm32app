
	mono "/Applications/Renode.app/Contents/MacOS/bin/Renode.exe"
    --hide-log
    -e include @scripts/single-node/stm32f4_discovery.resc
    -e machine StartGdbServer 3333 True
    -e logLevel 3           ;Loglevel = Error
    -e emulation CreateServerSocketTerminal 1234 "externalUART"
    -e connector Connect uart2 externalUART
    -e showAnalyzer uart2