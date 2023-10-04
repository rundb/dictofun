# Battery measurement

Battery measurement is taken to a separate task due to the importance of this function.

Basically this task performs 3 functions:
- perform measurements of battery voltage level (so it configures ADC and performs filtering)
- perform a conversion of the collected samples into an estimation of voltage level
- provide the battery level estimation to the consumers.