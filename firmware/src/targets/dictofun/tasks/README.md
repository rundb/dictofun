# Tasks of dictofun application

The methods implemented here should be later moved to RTOS tasks

## Application state machine

Main application works based on the following state machine:

```plantumlcode
@startuml
[*] --> Init
Init : Startup the HW
Init -> Prepare : done

Prepare : initialize FS, prepare to record
Prepare -> Record : done

Record: perform the record
Record --> Connect : button released

Connect: stop the record, close the written file, establish BLE connection
Connect --> Transfer : connection established
Connect -> Restart : button pressed

Transfer: send the saved file(s) to the phone
Transfer --> Disconnect : done
Transfer --> Disconnect : timeout
Transfer -> Restart : button pressed

Disconnect: finalize the transfer, close BLE connection
Disconnect --> Finalize : done
Disconnect -> Restart : button pressed

Finalize: erase the flash memory, clean the FS descriptors, prepare for a fast restart
Finalize -> Restart : button pressed

Restart-->Record : preparation done

Connect-->Finalize: timeout

Finalize --> [*]
@enduml
```
