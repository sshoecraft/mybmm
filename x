2021-03-31 15:26:02  read_config(56): cfg: 0xd694f8
[mybmm]                    debug: 3
[mybmm]                 interval: 20
[mybmm]          max_charge_amps: -1.000000
[mybmm]       max_discharge_amps: -1.000000
[mybmm]           system_voltage: 48
[mybmm]             battery_chem: 1
[mybmm]                    cells: 14
[mybmm]                 cell_low: 3.200000
[mybmm]            cell_crit_low: -1.000000
[mybmm]                cell_high: 4.000000
[mybmm]           cell_crit_high: -1.000000
[mybmm]                   c_rate: -1.000000
[mybmm]                   e_rate: -1.000000
[mybmm]                 capacity: 1050.000000
[mybmm]           charge_voltage: -1.000000
[mybmm]              charge_amps: 120.000000
[mybmm]        discharge_voltage: -1.000000
[mybmm]           discharge_amps: -1.000000
[mybmm]       charge_max_voltage: -1.000000
[mybmm]            charge_at_max: yes
[mybmm]                      soc: -1.000000
[mybmm]         use_pack_voltage: no
[mqtt]         broker: localhost
[mqtt]          topic: /Powerwall
[mqtt]       username: 
[mqtt]       password: 
2021-03-31 15:26:02  read_config(72): db_name:
2021-03-31 15:26:02  battery_init(13): battery_chem: LITHIUM
2021-03-31 15:26:02  battery_init(51): charge_max_voltage: 58.8
2021-03-31 15:26:02  battery_init(53): charge_target_voltage: 56.0
2021-03-31 15:26:02  battery_init(54): cell_low: 3.2
2021-03-31 15:26:02  battery_init(55): cell_crit_low: 3.0
2021-03-31 15:26:02  battery_init(56): cell_high: 4.0
2021-03-31 15:26:02  battery_init(57): cell_crit_high: 4.2
2021-03-31 15:26:02  battery_init(58): c_rate: 0.7
2021-03-31 15:26:02  battery_init(59): e_rate: 1.0
2021-03-31 15:26:02  charge_stop(74): user_discharge_voltage: -1.0, user_discharge_amps: -1.0
2021-03-31 15:26:02  charge_stop(76): conf->e_rate: 1.000000, conf->capacity: 1050.000000
2021-03-31 15:26:02  charge_start(86): charge_at_max: 1
2021-03-31 15:26:02  charge_start(93): conf->c_rate: 0.700000, conf->capacity: 1050.000000
2021-03-31 15:26:02  charge_start(99): start_temp: 25.0
2021-03-31 15:26:02  get_tab(169): name: inverter
[inverter]            name: mybmm
[inverter]            uuid: 93016702-9a54-4592-8610-ba28a6d25917
[inverter]            type: si
[inverter]       transport: can
[inverter]          target: can0,500000
[inverter]          params: 
2021-03-31 15:26:02  mybmm_load_module(58): NOT found.
2021-03-31 15:26:02  mybmm_load_module(103): module: 0x452ac
2021-03-31 15:26:02  mybmm_load_module(111): init: 0x26a80
2021-03-31 15:26:02  mybmm_load_module(118): adding module: can
2021-03-31 15:26:02  mybmm_load_module(52): mp->name: can, mp->type: 2
2021-03-31 15:26:02  mybmm_load_module(58): NOT found.
2021-03-31 15:26:02  mybmm_load_module(103): module: 0x451dc
2021-03-31 15:26:02  mybmm_load_module(111): init: 0x21570
2021-03-31 15:26:02  mybmm_load_module(118): adding module: si
2021-03-31 15:26:02  inverter_add(204): mp: 0x451dc
2021-03-31 15:26:02  si_new(122): transport: can
2021-03-31 15:26:02  can_new(45): target: can0,500000
2021-03-31 15:26:02  can_new(58): interface: can0, bitrate: 500000
2021-03-31 15:26:02  si_new(152): setting func to local data
2021-03-31 15:26:02  si_thread(36): thread started!
2021-03-31 15:26:02  inverter_add(214): capabilities: 07
2021-03-31 15:26:02  inverter_add(227): done!
2021-03-31 15:26:02  pack_init(371): name: pack_01
2021-03-31 15:26:02  read_config(96): conf: dirty? 0
[mqtt]         broker: localhost
[mqtt]          topic: /Powerwall
[mqtt]       username: 
[mqtt]       password: 
2021-03-31 15:26:02  init(133): battery_chem: -1
2021-03-31 15:26:02  *** STARTING CC CHARGE ***
2021-03-31 15:26:02  charge_start(86): charge_at_max: 1
2021-03-31 15:26:02  charge_start(93): conf->c_rate: 0.700000, conf->capacity: 1050.000000
2021-03-31 15:26:02  Charge voltage: -1.0, Charge amps: 120.0
2021-03-31 15:26:02  charge_start(99): start_temp: 25.0
2021-03-31 15:26:02  main(201): startup: 1
2021-03-31 15:26:02  si_open(169): opening transport
2021-03-31 15:26:02  can_open(784): opening socket...
2021-03-31 15:26:02  can_open(790): fd: 3
2021-03-31 15:26:02  can_open(801): if_up: 1
2021-03-31 15:26:02  can_open(817): current bitrate: 500000, wanted bitrate: 500000
2021-03-31 15:26:02  can_open(860): done!
2021-03-31 15:26:02  si_open(171): r: 0
