
void button_states(int analog_val, boolean button_input[]){
  float levels[32] = {14,33.5,52.5,73,90,110,131,152.5,164,184,204.5,
                    226,246,267,290,314,322,342,363,384.5,404.5,426,
                    449.5,474,491,512.5,536.5,561.5,586,610,640,672};
  
  int negmin = 0;
  byte neg_index = 0;
  int posmin = 0;
  byte pos_index = 0;
  byte index;

  for(int i = 0; i < 31 ; i++){
    posmin = analog_val - levels[i];
    pos_index = i;
    negmin = analog_val - levels[i+1];
    neg_index = i+1;
    if (negmin < 0)
      break;
  }

  if ((posmin+negmin)> 0){    //if negmin is closer to 0
    index = neg_index;
  }
  else{
    index = pos_index;
  }

  for(int i = 0; i < 5; i++){
    button_input[4-i] = bitRead(index, i);
  }
  
  
}


