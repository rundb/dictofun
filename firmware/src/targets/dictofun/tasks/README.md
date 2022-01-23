# Tasks of dictofun application

The methods implemented here should be later moved to RTOS tasks

## Application state machine

Main application works based on the following state machine:

```plantumlcode
@startuml
[*] --> Init
Init : Startup the HW
Init --> Prepare : done
Init -> Finalize: error during init

Prepare : initialize FS, prepare to record

Prepare --> Rec.Start
Rec.Start : start the record
Rec.Start --> Record
Prepare -> Finalize: error during preparation
Record --> Rec.finalization
Record: perform the recording

Rec.finalization --> Connect

Rec.finalization: should be atomic

Connect: stop the record, close the written file, establish BLE connection
Connect --> Transfer : connection established
Connect --> Restart : button pressed
Connect->Finalize: timeout

Transfer: send the saved file(s) to the phone
Transfer --> Disconnect : done
Transfer --> Disconnect : timeout
Transfer --> Restart : button pressed

Disconnect: finalize the transfer, close BLE connection
Disconnect -> Finalize : done
Disconnect --> Restart : button pressed

Finalize: erase the flash memory, clean the FS descriptors, prepare for a fast restart
Finalize --> Restart : button pressed

Restart-->Rec.Start : preparation done

Finalize --> [*]
@enduml
```
