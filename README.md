# OpenP1

Open source reader for (mainly) Swedish electricity meters with P1 port.

Exposes data read from meter as a Modbus TCP or UDP server on a Thread network.

The software is in an early stage, and usage is not recommended unless you know what you are doing.

# Suggested Circuit

TODO

# Suggested way of collecting and presenting data

## Sample config for Home assistant
```
- name: "blep1-dongle"
  close_comm_on_error: true
  type: tcp
  host: blep-device
  port: 502
  sensors:
    - name: p1_meter_energy_in
      unique_id: p1_dongle1_meter_energy_in
      slave: 1
      address: 2080
      input_type: input
      data_type: uint32
      scan_interval: 15
      unit_of_measurement: kWh
      scale: 0.001
      precision: 3
      state_class: total_increasing
      device_class: energy
    - name: p1_meter_energy_out
      unique_id: p1_dongle1_meter_energy_out
      slave: 1
      address: 2112
      input_type: input
      data_type: uint32
      scan_interval: 15
      unit_of_measurement: kWh
      scale: 0.001
      precision: 3
      state_class: total_increasing
      device_class: energy
    - name: p1_power_in
      unique_id: p1_dongle1_power_in
      slave: 1
      address: 2208
      input_type: input
      data_type: uint32
      scan_interval: 15
      unit_of_measurement: W
      state_class: measurement
      device_class: power
    - name: p1_power_out
      unique_id: p1_dongle1_power_out
      slave: 1
      address: 2240
      input_type: input
      data_type: uint32
      scan_interval: 15
      unit_of_measurement: W
      state_class: measurement
      device_class: power
```


# References

https://www.energiforetagen.se/globalassets/energiforetagen/det-erbjuder-vi/kurser-och-konferenser/elnat/branschrekommendation-lokalt-granssnitt-v2_0-201912.pdf
https://hanporten.se/uit
