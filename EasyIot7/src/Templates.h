/*
#if defined SINGLE_SWITCH
    SwitchT one;
    templateSwitch(one, "Interruptor", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    switches.items.push_back(one);
#elif defined DUAL_LIGHT
    SwitchT one;
    SwitchT two;
    templateSwitch(one, "Interruptor 1", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    one.knxLevelOne = 0;
    one.knxLevelTwo = 0;
    one.knxLevelThree = 0;
    switches.items.push_back(one);
    templateSwitch(two, "Interruptor 2", constanstsSwitch::familyLight, SWITCH, 13u, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    two.knxLevelOne = 0;
    two.knxLevelTwo = 0;
    two.knxLevelThree = 0;
    switches.items.push_back(two);

#elif defined FOUR_LOCK
    SwitchT one;
    SwitchT two;
    SwitchT three;
    SwitchT four;
    templateSwitch(one, "Porta 1", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 14u, constantsConfig::noGPIO);
    templateSwitch(two, "Porta 2", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    templateSwitch(three, "Porta 3", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 12u, constantsConfig::noGPIO);
    templateSwitch(four, "Porta 4", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);

    one.autoStateDelay = 1000; //1 second
    strlcpy(one.autoStateValue, constanstsSwitch::payloadOff, sizeof(one.autoStateValue));
    two.autoStateDelay = 1000; //1 second
    strlcpy(two.autoStateValue, constanstsSwitch::payloadOff, sizeof(two.autoStateValue));
    three.autoStateDelay = 1000; //1 second
    strlcpy(three.autoStateValue, constanstsSwitch::payloadOff, sizeof(three.autoStateValue));
    four.autoStateDelay = 1000; //1 second
    strlcpy(four.autoStateValue, constanstsSwitch::payloadOff, sizeof(four.autoStateValue));
    switches.items.push_back(one);
    switches.items.push_back(two);
    switches.items.push_back(three);
    switches.items.push_back(four);
#elif defined VMC
    SwitchT one;
    SwitchT two;

    templateSwitch(one, "VMC", constanstsSwitch::familySwitch, PUSH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    one.autoStateDelay = 45 * 60 * 1000; //45 minutes

    strlcpy(one.autoStateValue, constanstsSwitch::payloadOff, sizeof(one.autoStateValue));

    templateSwitch(two, "BOMBA", constanstsSwitch::familySwitch, PUSH, 12u, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    two.autoStateDelay = 3 * 60 * 1000; //45 minutes

    strlcpy(two.autoStateValue, constanstsSwitch::payloadOff, sizeof(two.autoStateValue));

    switches.items.push_back(one);
    switches.items.push_back(two);

#elif defined COVER
    SwitchT one;
    templateSwitch(one, "Estore", constanstsSwitch::familyCover, DUAL_SWITCH, 12u, 13u, 4u, 5u);
    switches.items.push_back(one);
#elif defined LOCK
    SwitchT one;
    templateSwitch(one, "Portão", constanstsSwitch::familyLock, PUSH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    switches.items.push_back(one);
#elif defined GATE
    SwitchT one;
    templateSwitch(one, "Portão", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    one.autoStateDelay = 1000; //1 second
    strlcpy(one.autoStateValue, constanstsSwitch::payloadReleased, sizeof(one.autoStateValue));
    switches.items.push_back(one);
#endif
*/