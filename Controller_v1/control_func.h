void button_states_2(int analog_val, boolean button_input[]){
  float levels[33] = {0,24.5,47,69.5,91.5,114,138,160.5,178,198.5,224,248,270.5,293.5,317.5,
                      342,359,375,398.5,422.5,445.5,468.5,493,468.5,490,561,586,611,635.5,
                      661,689,719.5,1024};
  byte index;
  for(int i = 0; i < 32; i++){
    if (analog_val >= levels[i] && analog_val < levels[i+1]){
      index = i;
      break;
    }
  }

  for(int i = 0; i < 5; i++){
    button_input[4-i] = bitRead(index, i);
  }
}


//off     13
//rbump   36
//lbump   58
//L+R     81
//
//white   102
//w+R     126
//w+L     150
//w+L+R   171
//
//blue    185
//b+R     212
//b+L     236
//b+R+L   260
//b+w     281
//b+w+R   306
//b+w+L   329
//b+w+R+L 355
//
//green   363
//gR      387
//gL      410
//gLR     435
//gw      456
//gwR     481
//gwL     505
//gwLR    532
//gb      548
//gbR     574
//gbL     598
//gbLR    624
//gbw     647
//gbwR    675
//gbwL    703
//gbwLR   736




