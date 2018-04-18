// demo driver example for libPCA9685
// copyright 2016 Scott Edlin

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <ncurses.h>

#include <PCA9685.h>

//#define DEBUG
#define VALIDATE
#define NCMODE


int fd;
int addr = 0x40;


void cleanup() {
  // attmempt to turn off all PWM
  PCA9685_setAllPWM(fd, addr, _PCA9685_MINVAL, _PCA9685_MINVAL);
  #ifdef NCMODE
  // attmempt to end the ncurses window session
  endwin();
  #endif
} // cleanup



void intHandler(int dummy) {
  cleanup();
  fprintf(stdout, "Caught signal, exiting\n");
  exit(0);
} // intHandler 


int initHardware(int adpt, int addr, int freq) {
  int fd;
  int ret;

  // setup the I2C bus device 
  fd = PCA9685_openI2C(adpt, addr);
  if (fd < 0) {
    fprintf(stderr, "initHardware(): PCA9685_openI2C() returned ");
    fprintf(stderr, "%d for adpt %d at addr %x\n", fd, adpt, addr);
    return -1;
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = PCA9685_dumpAllRegs(fd, addr);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): PCA9685_dumpAllRegs() returned %d\n", ret);
    return -1;
  } // if 
  #endif

  // initialize the pca device 
  ret = PCA9685_initPWM(fd, addr, freq);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): PCA9685_initPWM() returned %d\n", ret);
    return -1;
  } // if 

  #ifdef DEBUG
  // display all used pca registers 
  ret = PCA9685_dumpAllRegs(fd, addr);
  if (ret != 0) {
    fprintf(stderr, "initHardware(): PCA9685_dumpAllRegs() returned %d\n", ret);
    return -1;
  } // if 
  #endif

  return fd;
} // initHardware


int dumpStats(int stats[]) {
  int i;
  for(i=0; i<8; i++) {
    #ifdef NCMODE
    mvprintw(1, i*10, "%-10d", stats[i]);
    #else
    printf("%-10d", stats[i]);
    if (i==7) { printf("\n"); }
    #endif
  } // for

  return 0;
} // dumpStats


int dumpVals(unsigned int row, unsigned int* vals, unsigned char chan) {
  unsigned char i;
  for(i=0; i<_PCA9685_CHANS; i++) {

    #ifdef NCMODE
    unsigned int colorVal;
    colorVal = (unsigned int)(vals[i] / 682.667f) + 1;

    if (i == chan) {
      attron(A_BOLD | COLOR_PAIR(colorVal));
    } // if
    else {
      attron(A_NORMAL | COLOR_PAIR(colorVal));
    } // else

    mvprintw(row, 5*i, "%03x", vals[i]);
    attroff(COLOR_PAIR(colorVal));
    attroff(A_BOLD);

    #else
    printf("%03x  ", vals[i]);
    if (i==_PCA9685_CHANS) { printf("\n"); };

    #endif
  } // for

  #ifdef NCMODE
  refresh();
  #endif

  return 0;
} // dumpVals


void dumpRegs(unsigned char mode1val, unsigned char mode2val) {
  mvprintw(6, 0, "%02x %02x", mode1val, mode2val);
} // dumpRegs



int initScreen() {

  #ifdef NCMODE
  int ret;
  WINDOW* wndw;

  // start ncurses mode
  wndw = initscr();
  if (wndw != stdscr) {
    fprintf(stderr, "initScreen(): initscr() did not return stdscr\n");
    return -1;
  } // if

  // no line buffering but allow control chars like CTRL-C
  cbreak();

  // enable extended keys
  keypad(stdscr, TRUE);

  // no echo
  noecho();

  // initialize ncurses colors
  ret = start_color();
  if (ret == ERR) {
    fprintf(stderr, "initScreen(): start_color() returned ERR\n");
    return -1;
  } // if err

  // define color pairs in opposite ROYGBIV order
  init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(2, COLOR_BLUE, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_GREEN, COLOR_BLACK);
  init_pair(5, COLOR_YELLOW, COLOR_BLACK);
  init_pair(6, COLOR_RED, COLOR_BLACK);

  // header row for stats
  mvprintw(0,  0, "frames");
  mvprintw(0, 10, "bits");
  mvprintw(0, 20, "ms");
  mvprintw(0, 30, "avg");
  mvprintw(0, 40, "Hz");
  mvprintw(0, 50, "avg");
  mvprintw(0, 60, "kbps");
  mvprintw(0, 70, "avg");

  // dump off values to start
  dumpVals(3, (unsigned int[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 0);

  #ifdef VALIDATE
  dumpVals(4, (unsigned int[]){0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, -1);
  #endif

  refresh();

  #else
  printf("frames    bits      ms        avg       Hz        avg       kbps      avg\n");

  #endif

  return 0;
} // initScreen


struct rgb {
  int r;
  int g;
  int b;
}; // typedef


struct hsv {
  float h;
  float s;
  float v;
}; // typedef


#define MAXRGBVAL _PCA9685_MAXVAL


struct rgb hsv2rgb(struct hsv _hsv) {
  struct rgb _rgb;
  float var_r, var_g, var_b;

  if (_hsv.s <= 0.0) {
    _rgb.r = (int)(_hsv.v * MAXRGBVAL);
    _rgb.g = (int)(_hsv.v * MAXRGBVAL);
    _rgb.b = (int)(_hsv.v * MAXRGBVAL);
  } // if

  else {
    float var_h = _hsv.h * 6;
    if (var_h >= 6.0) var_h = 0;
    int var_i = (int)var_h;
    float var_1 = _hsv.v * (1 - _hsv.s);
    float var_2 = _hsv.v * (1 - _hsv.s * (var_h - var_i));
    float var_3 = _hsv.v * (1 - _hsv.s * (1 - (var_h - var_i)));

    if      (var_i == 0) { var_r = _hsv.v; var_g = var_3;  var_b = var_1;  }
    else if (var_i == 1) { var_r = var_2;  var_g = _hsv.v; var_b = var_1;  }
    else if (var_i == 2) { var_r = var_1;  var_g = _hsv.v; var_b = var_3;  }
    else if (var_i == 3) { var_r = var_1;  var_g = var_2;  var_b = _hsv.v; }
    else if (var_i == 4) { var_r = var_3;  var_g = var_1;  var_b = _hsv.v; }
    else                 { var_r = _hsv.v; var_g = var_1;  var_b = var_2;  }

    _rgb.r = (int)(var_r * MAXRGBVAL);
    _rgb.g = (int)(var_g * MAXRGBVAL);
    _rgb.b = (int)(var_b * MAXRGBVAL);

  } // else

  return _rgb;
} // hsv2rgb







// main driver 
int main(void) {
  int adpt = 1;
  int freq = 200;
  int ret;
  char automatic;
  char manual;

  #ifdef NCMODE
  automatic = false;
  manual = true;
  #else
  automatic = true;
  manual = false;
  #endif

  // register the signal handler to catch interrupts
  signal(SIGINT, intHandler);

  // initialize the I2C bus adpt and a PCA9685 at addr with freq
  fd = initHardware(adpt, addr, freq);
  if (fd < 0) {
    cleanup();
    fprintf(stderr, "main(): initHardware() returned ");
    fprintf(stderr, "%d for adpt %d at addr %02x\n", fd, adpt, addr);
    return -1;
  } // if

  // initialize the screen
  ret = initScreen();
  if (ret < 0) {
    cleanup();
    fprintf(stderr, "main(): initScreen() returned %d\n", ret);
    return -1;
  } // if

  unsigned char mode1val;
  unsigned char mode2val;
  ret = PCA9685_getRegVals(fd, addr, &mode1val, &mode2val);
  if (ret != 0) {
    cleanup();
    fprintf(stderr, "main(): PCA9685_GetRegVals() returned ");
    fprintf(stderr, "%d for adpt %d at addr %02x\n", ret, adpt, addr);
    return -1;
  } // if
  dumpRegs(mode1val, mode2val);

  { // perf context 
    int j = 0;
    int chan = 0;
    int steps[_PCA9685_CHANS];
    int minStep = 10;
    int maxStep = 20;
    unsigned int setOnVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int setOffVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    #ifdef VALIDATE
    unsigned int getOnVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int getOffVals[_PCA9685_CHANS] =
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    #endif

    int size = 512;
    int bits;
    int mema = 0;
    int fema = 0;
    int kema = 0;
    float alpha = 0.1;
    float invalpha = 1.0 - alpha;
    struct timeval then, now, diff;

    gettimeofday(&then, NULL);
    
    // initialize steps for automatic mode
    int i;
    for (i=0; i<_PCA9685_CHANS; i++) {
      steps[i] = rand() % maxStep + minStep;
    } // for 

    int c;
    struct rgb RGB;
    struct hsv HSV;
    HSV.h = 0.0;
    HSV.s = 1.0;
    HSV.v = 1.0;

    // blink endlessly 
    while (1) {

      if (automatic) {
        // read a char (non-blocking)
        c = getch();
        // if m, switch to manual
        if (c != ERR) {
          if (c == (int)('m')) {
            automatic = false;
            manual = true;
          } // if
        } // if

        for (i=0; i<_PCA9685_CHANS; i++) {

          HSV.h += 0.0001;
          if (HSV.h >= 1.0) HSV.h = 0.0;
          RGB = hsv2rgb(HSV);
          setOffVals[i*3+0] = RGB.r;
          setOffVals[i*3+1] = RGB.g;
          setOffVals[i*3+2] = RGB.b;
        } // for 
      } // if auto

      else if (manual) {
        int c;
        int upStep = 16;

        // read a char (blocking)
        nodelay(stdscr, false);
        c = getch();

        // if left, chan--
        if (c == KEY_LEFT) {
          chan = (chan > 0) ? chan-1 : _PCA9685_CHANS;
        } // if

        // else if right, chan++
        else if (c == KEY_RIGHT) {
          chan = (chan < _PCA9685_CHANS) ? chan+1 : 0;
        } // if

        // else if up, boost vals[chan]
        else if (c == KEY_UP) {
          setOffVals[chan] = (setOffVals[chan]  > _PCA9685_MAXVAL - upStep)
                       ? _PCA9685_MAXVAL : setOffVals[chan] + upStep;
        } // if

        // else if down, drop vals[chan]
        else if (c == KEY_DOWN) {
          setOffVals[chan] = (setOffVals[chan] < _PCA9685_MINVAL + upStep)
                       ? _PCA9685_MINVAL : setOffVals[chan] - upStep;
        } // if

        // else if period, inc vals[chan]
        else if (c == (int)('.')) {
          setOffVals[chan] = (setOffVals[chan] > _PCA9685_MAXVAL - 1)
                       ? _PCA9685_MAXVAL : setOffVals[chan] + 1;
        } // if

        // else if down, drop vals[chan]
        else if (c == (int)(',')) {
          setOffVals[chan] = (setOffVals[chan] < _PCA9685_MINVAL + 1)
                       ? _PCA9685_MINVAL : setOffVals[chan] - 1;
        } // if

        // else if left bracket, set chan to minimum val
        else if (c == (int)('[')) {
          setOffVals[chan] = _PCA9685_MINVAL;
        } // if

        // else if right bracket, set chan to maximum val
        else if (c == (int)(']')) {
          setOffVals[chan] = _PCA9685_MAXVAL;
        } // if

        // else if a, switch to automatic
        else if (c == (int)('a')) {
          manual = false;
          automatic = true;
          gettimeofday(&then, NULL);
          nodelay(stdscr, true);
        } // if

        // else if 0, set to minimum val
        else if (c == (int)('0')) {
          int i;
          for(i=0; i<_PCA9685_CHANS; i++) {
            setOffVals[i] = _PCA9685_MINVAL;
          } // for
        } // if

        // else if 1, set to maximum val
        else if (c == (int)('1')) {
          int i;
          for(i=0; i<_PCA9685_CHANS; i++) {
            setOffVals[i] = _PCA9685_MAXVAL;
          } // for
        } // if
        // TODO: else if s, toggle strobe
      } // if manual


      // SET THE VALS, EVERY TIME THROUGH THE LOOP
      ret = PCA9685_setPWMVals(fd, addr, setOnVals, setOffVals);
      if (ret != 0) {
        cleanup();
        fprintf(stderr, "main(): PCA9685_setPWMVals() returned ");
        fprintf(stderr, "%d on addr %02x\n", ret, addr);
        return -1;
      } // if 

      #ifdef VALIDATE
      // GET THE VALS, EVERY TIME THROUGH THE LOOP
      ret = PCA9685_getPWMVals(fd, addr, getOnVals, getOffVals);
      if (ret != 0) {
        cleanup();
        fprintf(stderr, "main(): PCA9685_getPWMVals() returned ");
        fprintf(stderr, "%d on addr %02x at reg 0x06\n", ret, addr);
        return -1;
      } // if err

      // compare the written vals to the read vals
      for(int i=0; i<_PCA9685_CHANS; i++) {
        if (getOnVals[i] != setOnVals[i]) {
          cleanup();
          fprintf(stderr, "main(): getOnVals[%d] is %03x ", i, getOnVals[i]);
          fprintf(stderr, "but setOnVals[%d] is %03x\n", i, setOnVals[i]);
          exit(-1-i);
        } // if data mismatch
        if (getOffVals[i] != setOffVals[i]) {
          cleanup();
          fprintf(stderr, "main(): getOffVals[%d] is %03x ", i, getOffVals[i]);
          fprintf(stderr, "but setOffVals[%d] is %03x\n", i, setOffVals[i]);
          exit(-1-i);
        } // if data mismatch
      } // for channels
      #endif

      // increment the loop counter
      j++;

      if (manual || j >= size) {
        int stats[8];
        long int micros;
        int millis;
        int updfreq;
        int kbps;
        j = 0;

        if (automatic) {
          gettimeofday(&now, NULL);
          timersub(&now, &then, &diff);
          then = now;

          micros = (long int)diff.tv_sec*1000000 + (long int)diff.tv_usec;
          millis = micros / 1000;
          millis = (millis == 0) ? 1 : millis;
          updfreq = (int)((float)1000000 * (float)size) / (float)micros;
          bits = size * _PCA9685_CHANS * 4 * 8;
          #ifdef VALIDATE
          bits *= 2;
          #endif
          kbps = (bits * 1000000.0f) / (micros * 1024.0f);

          fema = (fema == 0) ? updfreq
            : (alpha * updfreq) + (invalpha * fema);
          mema = (mema == 0) ? millis
            : (alpha * millis) + (invalpha * mema);
          kema = (kema == 0) ? kbps
            : (alpha * kbps) + (invalpha * kema);

          stats[0] = size;
          stats[1] = bits;
          stats[2] = millis;
          stats[3] = mema;
          stats[4] = updfreq;
          stats[5] = fema;
          stats[6] = kbps;
          stats[7] = kema;

          dumpStats(stats);
        } // if automatic

        dumpVals(3, setOffVals, chan);

        #ifdef VALIDATE
        dumpVals(4, getOffVals, -1);
        #endif

      } // if update screen

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 18000;
      select((int)NULL, NULL, NULL, NULL, &timeout);
    } // while forever
  } // perf context 

  return 0;
} // main 
