# File system requirements for the firmware

* File system should be able to write, read and delete raw audio files into external NOR SPI Flash memory

* File system should support erase operation

* File opening for write should be available after startup of the device within 400 ms. 

* Read, write and erase operations should be asynchronous

* Only files should be supported (no directories support)
  
* Abort of erase operation should be supported

* Write of raw data into a fallback segment should be supported

The last two requirements are necessary for the use-case when record button is pressed during the `finalize` stage of the FSM.
In this case we can have a long-running erase operation that has to be cancelled ASAP, so we can keep within the requirement
of max 400 ms before the record starts.
