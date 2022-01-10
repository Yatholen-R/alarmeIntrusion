// flanc montant
class PositivEdge {
  private :
    boolean memPrevState;
    boolean out;
  public :
    PositivEdge(boolean condition);                      //constructor
    boolean eval(boolean condition);
    boolean get_out();
};
PositivEdge::PositivEdge(boolean condition) {
  this->memPrevState = condition;
}
boolean PositivEdge::eval(boolean condition) { //update positiv edge state must be done ONLY ONCE by loop cycle
  out = condition && !memPrevState;
  memPrevState = condition;
  return out;
}
boolean PositivEdge::get_out() {  //use get_out() to know positiv edge state (use more than once by cycle is possible)
  return this < - out;
} // fin flanc montant

class OnDelayTimer {

  private :
    unsigned long presetTime = 1000;
    unsigned long memStartTimer = 0;            //memory top timer activation
    unsigned long elpasedTime = 0;             //elapsed time from start timer activation
    boolean memStartActivation;                //for positive edge detection of the activation condition
    boolean outTimer;                          //timer's out : like normally open timer switch
  public :
    OnDelayTimer(unsigned long _presetTime);   //constructor
    boolean updtTimer(boolean activation);      //return tempo done must be executed on each program scan
    unsigned long get_elapsedTime();           //return
    set_presetTime(unsigned long _presetTime); //change defaut preset assigned when instance created
    boolean get_outTimer();

};//end class OnDelayTimer
//constructor
OnDelayTimer::OnDelayTimer(unsigned long presetTime) {
  this -> presetTime = presetTime;
}
boolean OnDelayTimer::updtTimer(boolean activation) {
  if (!activation) {
    elpasedTime = 0;
    memStartActivation = false;
    outTimer = false;
    return false;
  } else {

    if (!memStartActivation) {
      memStartTimer = millis();
      memStartActivation = true;
    }
    elpasedTime = millis() - memStartTimer;
    outTimer = elpasedTime >= this->presetTime; //update timer 's "switch"
    return  outTimer;

  }
}//end endTimer()
//constructor
boolean OnDelayTimer::get_outTimer() {

  return this->outTimer;
}

// pin de sortie 22->27

const int iPIN_buz = 24;
const int iPIN_sext = 25;
const int iPIN_H1 = 23;
const int iPIN_H2 = 22;

//pin entrée NO 40->46
const int iPIN_arm = 40;
const int iPIN_valid = 41;
const int iPIN_touche1 = 42;
const int iPIN_touche2 = 43;
const int iPIN_touche3 = 44;
const int iPIN_touche4 = 45;

// pin entrée NF 47->49
const int iPIN_intrus = 47;

boolean buz, sext, H1, H2 = 0; //boolean sortie
boolean arm, valid, touche1, touche2, touche3, touche4, intrus = 0; //boolean entrée
//variable additionel
boolean codeValide = 0;
unsigned int cptWrong = 0;

// nombre étapes et transistions
const unsigned int nbStepPr = 4;
const unsigned int nbTransition = 6;
boolean stepPr[nbStepPr];
boolean transition[nbTransition];
boolean transitionCode[10];
boolean stepCode[6];

//déclaration débug
String strDebugLine;
int stp, stpCode; // étape numéro x dans le debug

// déclaration des flanc montant à getter: PositivEdge nomDeVariable(nomDeVariable à évaluer)
PositivEdge posEdge_arm(arm);
PositivEdge posEdge_touche1(touche1);
PositivEdge posEdge_touche2(touche2);
PositivEdge posEdge_touche3(touche3);
PositivEdge posEdge_touche4(touche4);
PositivEdge posEdge_valid(valid);
PositivEdge posEdge_stepPr0(stepPr[0]);
PositivEdge posEdge_stepCode5(stepCode[5]);

// déclaration timer : OnDelayTimer nomDeVariable(temps en milliseconde);
OnDelayTimer timerStepPr2(40000);
OnDelayTimer timerStepPr3(600000);

void setup() {
  Serial.begin(9600); // déclaration moniteur série
  // sortie
  pinMode(iPIN_buz, OUTPUT);
  pinMode(iPIN_sext, OUTPUT);
  pinMode(iPIN_H1, OUTPUT);
  pinMode(iPIN_H2, OUTPUT);
  //entrée
  pinMode(iPIN_arm, INPUT);
  pinMode(iPIN_valid, INPUT);
  pinMode(iPIN_touche1, INPUT);
  pinMode(iPIN_touche2, INPUT);
  pinMode(iPIN_touche3, INPUT);
  pinMode(iPIN_touche4, INPUT);

  pinMode(iPIN_intrus, INPUT);

  stepPr[0] = true; // début stepPr
  stepCode[0] = true;
}

void loop() {
  //lecture entrée
  arm = digitalRead (iPIN_arm);
  valid = digitalRead (iPIN_valid);
  touche1 = digitalRead (iPIN_touche1);
  touche2 = digitalRead (iPIN_touche2);
  touche3 = digitalRead (iPIN_touche3);
  touche4 = digitalRead (iPIN_touche4);
  intrus = digitalRead (iPIN_intrus);

  //evaluation flanc montant: posEdge_nomDeVariable.eval(nomDeVariable)
  posEdge_arm.eval(arm);
  posEdge_touche1.eval(touche1);
  posEdge_touche2.eval(touche2);
  posEdge_touche3.eval(touche3);
  posEdge_touche4.eval(touche4);
  posEdge_valid.eval(valid);
  posEdge_stepPr0.eval(stepPr[0]);
  posEdge_stepCode5.eval(stepCode[5]);

  // déclaration des transitions
  transition[0] = stepPr[0] && posEdge_arm.get_out();
  transition[1] = stepPr[1] && (!intrus || posEdge_touche1.get_out() || posEdge_touche2.get_out() || posEdge_touche3.get_out() || posEdge_touche4.get_out());
  transition[2] = stepPr[2] && timerStepPr2.get_outTimer();
  transition[3] = stepPr[3] && timerStepPr3.get_outTimer();
  transition[4] = (stepPr[1] || stepPr[2]) && (cptWrong >= 3);
  transition[5] = !stepPr[0] && codeValide && (cptWrong < 6);

  //stepPr étape alarme
  if (transition[0]) {
    stepPr[0] = false;
    stepPr[1] = true;
  }
  if (transition[1]) {
    stepPr[1] = false;
    stepPr[2] = true;
  }
  if (transition[2]) {
    stepPr[2] = false;
    stepPr[3] = true;
  }
  if (transition[3]) {
    stepPr[3] = false;
    stepPr[0] = true;
  }
  if (transition[4]) {
    stepPr[1] = false;
    stepPr[2] = false;
    stepPr[3] = true;
  }
  if (transition[5]) {
    stepPr[1] = false;
    stepPr[2] = false;
    stepPr[3] = false;
    stepPr[0] = true;
  }

  // Step input clavier
  transitionCode[0] = stepCode[0] && posEdge_touche4.get_out();
  transitionCode[1] = stepCode[1] && posEdge_touche2.get_out();
  transitionCode[2] = stepCode[2] && posEdge_touche1.get_out();
  transitionCode[3] = stepCode[3] && posEdge_valid.get_out();
  transitionCode[4] = stepCode[0] && (posEdge_touche1.get_out() || posEdge_touche2.get_out() || posEdge_touche3.get_out());
  transitionCode[5] = stepCode[1] && (posEdge_touche1.get_out() || posEdge_touche3.get_out() || posEdge_touche4.get_out());
  transitionCode[6] = stepCode[2] && (posEdge_touche2.get_out() || posEdge_touche3.get_out() || posEdge_touche4.get_out());
  transitionCode[7] = stepCode[3] && (posEdge_touche1.get_out() || posEdge_touche2.get_out() || posEdge_touche3.get_out() || posEdge_touche4.get_out());
  transitionCode[8] = stepCode[5] && posEdge_valid.get_out();
  transitionCode[9] = stepCode[4];

 // step alarme
  if (transitionCode[0]) {
    stepCode[0] = false;
    stepCode[1] = true;
  }
  if (transitionCode[1]) {
    stepCode[1] = false;
    stepCode[2] = true;
  }
  if (transitionCode[2]) {
    stepCode[2] = false;
    stepCode[3] = true;
  }
  if (transitionCode[3]) {
    stepCode[3] = false;
    stepCode[4] = true;
  }
  if (transitionCode[4]) {
    stepCode[0] = false;
    stepCode[5] = true;
  }
  if (transitionCode[5]) {
    stepCode[1] = false;
    stepCode[5] = true;
  }
  if (transitionCode[6]) {
    stepCode[2] = false;
    stepCode[5] = true;
  }
  if (transitionCode[7]) {
    stepCode[3] = false;
    stepCode[5] = true;
  }
  if (transitionCode[8]) {
    stepCode[5] = false;
    stepCode[0] = true;
  }
  if (transitionCode[9]) {
    stepCode[4] = false;
    stepCode[0] = true;
  }

  //déclaration code
  if (posEdge_stepCode5.get_out()) {
    cptWrong ++;
  }
  if (posEdge_stepPr0.get_out()) {
    cptWrong = 0;
    codeValide = false;
  }
  if ( stepCode[4]) {
    cptWrong = 0;
    codeValide = true;
  }
  

  //sortie activée par Step (sortie = stepPr[x])
  buz = stepPr[2];
  sext = stepPr[3];
  H1 = !stepPr[0];
  H2 = stepPr[0];

  // timer update (s'active à l'étape x)
  timerStepPr2.updtTimer(stepPr[2]);
  timerStepPr3.updtTimer(stepPr[3]);

  //association Sortie-Pin
  digitalWrite (iPIN_buz, buz);
  digitalWrite (iPIN_sext, sext);
  digitalWrite (iPIN_H1, H1);
  digitalWrite (iPIN_H2, H2);

  //debug: étapes active
  for (int i = 0; i < nbStepPr; i++) {
    if (stepPr[i]) {
      stp = i;
      break;
    }
  }
  for (int i = 0; i < 6; i++) {
    if (stepCode[i]) {
      stpCode = i;
      break;
    }
  }

  // sortie debug
  strDebugLine = "stepPr:" + String(stp, DEC) + "stepCode:" + String(stpCode, DEC) +/*" stepArret:" + String(stpArret, DEC) +*/
                 " arm:" + String(arm, DEC) + " valid:" + String(valid, DEC) + " touche1:" + String(touche1, DEC) + " touche2:" + String(touche2, DEC) + " touche3:" + String(touche3, DEC) +  " touche4:" + String(touche4, DEC) +
                 " intrus:" + String(intrus, DEC) + " cptWrong:" + String(cptWrong, DEC) + 
                 " buz:" + String(buz, DEC) + " sext:" + String(sext, DEC) + " H1:" + String(H1, DEC) + " H2:" + String(H2, DEC)+ " codeValide:" + String(codeValide, DEC);
  Serial.println(strDebugLine);
}
