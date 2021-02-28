#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

/***************************************************************************************
 * A small demo for the ytGraph function set 
 * Platform: Arduino / ESP 
 * 
 * Armin Pressler 2021
 * https://github.com/ArminPP/ytGraph
 * 
 * 
 * 
 * MIT License
 * 
 * Changelog:
 * v0.5   initial version (28-Feb-2021)
****************************************************************************************/

#include <version.h>
#include <Arduino.h>
#include "ytGraph.h"
#include "ytG_logo.h"

#ifdef useM5STACK     // define 'useM5STACK' is located in ytGraph.h
#include <M5Stack.h>  // compiles with the M5Stack variant of TFT_eSPI () library
#else                 //
#include <TFT_eSPI.h> // and the original library as well
#endif

#ifdef useM5STACK
M5Display &TFT = M5.Lcd;
#else
TFT_eSPI TFT = TFT_eSPI();
#endif

TFT_eSprite Graph = TFT_eSprite(&TFT); // canvas of graph, static & dynamic (grid and lines)
TFT_eSprite xAxis = TFT_eSprite(&TFT); // canvas of scrolling x axis

unsigned long previousMillis = 0; // non blocking delay
struct sensors
{
  float temperature1;
  float temperature2;
  float temperature3;
  int8_t humidity;
};
sensors SensorBuffer[SAMPLE_COUNT + 1] = {0}; // n+1 = because 1st data point starts at x=0 !

int16_t oy1 = 0; // static, local or global var to store the last y position in graph
int16_t oy2 = 0; // is need after each drawing of the historical buffer!
int16_t oy3 = 0;
int16_t oy4 = 0;
int16_t ox1 = 0; // for each new drwaing of historical data, x axis starts at zero
int16_t ox2 = 0;
int16_t ox3 = 0;
int16_t ox4 = 0;
int16_t LastXGridLinePos = 0; // for dynamic scrolling - to add new gridline at proper place

char *getAllFreeHeap()
{
  static char freeH[80]{}; // returns the formated free heap space
  sprintf(freeH, "Size:%.2fkB Free:%.2fkB Min:%.2fkB Max:%.2fkB",
          ESP.getHeapSize() / 1024.0,
          ESP.getFreeHeap() / 1024.0,
          ESP.getMinFreeHeap() / 1024.0,
          ESP.getMaxAllocHeap() / 1024.0);
  return freeH;
}

void printFreeHeap() // for debugging issues
{
  // TFT.fillRect(0, 0, TFT.width(), GRAPH_Y_BOTTOM_POS - GRAPH_HEIGHT - 2, GRAPH_BGRND_COLOR); // clear text area
  TFT.setCursor(0, 0);
  TFT.setTextSize(1);
  TFT.setTextColor(TFT_WHITE);
  TFT.print(getAllFreeHeap());
  Serial.println(getAllFreeHeap());
}

void printDemoInfoText(const char *string)
{
  TFT.fillRect(0, X_AXIS_UPPER_Y + X_AXIS_HEIGTH + 2, TFT.width(), TFT.height() - X_AXIS_UPPER_Y, GRAPH_BGRND_COLOR); // clear text area
  TFT.setTextSize(1);
  TFT.setTextColor(TFT_GREEN);
  TFT.setTextDatum(MC_DATUM);
  TFT.drawString(string, (int)TFT.width() / 2, (int)TFT.height() - 35, 2); // bold text
  TFT.drawString(string, (int)TFT.width() / 2 - 1, (int)TFT.height() - 35, 2);
  TFT.drawString(string, (int)TFT.width() / 2, (int)TFT.height() - 35 + 1, 2);
}

void drawStaticGraphBuffer()
{
  // starting old value on y axis is 1st value from SensorBuffer (for optical reasons)
  oy1 = abs(lround((GRAPH_HEIGHT * SensorBuffer[0].temperature1) / (abs(GRAPH_Y_AXIS_MIN) + GRAPH_Y_AXIS_MAX)) - GRAPH_HEIGHT);
  oy2 = abs(lround((GRAPH_HEIGHT * SensorBuffer[0].temperature2) / (abs(GRAPH_Y_AXIS_MIN) + GRAPH_Y_AXIS_MAX)) - GRAPH_HEIGHT);
  oy3 = abs(lround((GRAPH_HEIGHT * SensorBuffer[0].temperature3) / (abs(GRAPH_Y_AXIS_MIN) + GRAPH_Y_AXIS_MAX)) - GRAPH_HEIGHT);
  oy4 = abs(lround((GRAPH_HEIGHT * SensorBuffer[0].humidity) / (abs(GRAPH_Y_AXIS_MIN) + GRAPH_Y_AXIS_MAX)) - GRAPH_HEIGHT);

  ox1 = 0; // for each new drwaing of historical data, x axis starts at zero!
  ox2 = 0;
  ox3 = 0;
  ox4 = 0;
  for (int8_t i = 0; i <= SAMPLE_COUNT; i++) // SAMPLE_COUNT = +1
  {
    ytGraph(Graph, i, lround(SensorBuffer[i].temperature1), TFT_CYAN, ox1, oy1);
    ytGraph(Graph, i, lround(SensorBuffer[i].temperature2), TFT_PINK, ox2, oy2);
    ytGraph(Graph, i, lround(SensorBuffer[i].temperature3), TFT_YELLOW, ox3, oy3);
    ytGraph(Graph, i, SensorBuffer[i].humidity, TFT_MAGENTA, ox4, oy4);

    Graph.pushSprite(SPRITE_LEFT_X, SPRITE_UPPER_Y); // left upper position
  }
}

void drawDynamicGraph()
{
  int16_t scrollX = round(GRAPH_WIDTH / SAMPLE_COUNT); // scroll one sample to the left

  ox1 -= scrollX; // correction of the last point in graph after scrolling to the left
  ox2 -= scrollX;
  ox3 -= scrollX;
  ox4 -= scrollX;

  Graph.scroll(-scrollX);
  xAxis.scroll(-scrollX);

  ytGraphDrawDynamicGrid(Graph, xAxis, ox4, LastXGridLinePos);

  // add to the the end of the buffer the latest sensor reading
  SensorBuffer[SAMPLE_COUNT].temperature1 = lround(random(15, 25));
  SensorBuffer[SAMPLE_COUNT].temperature2 = lround(random(30, 45));
  SensorBuffer[SAMPLE_COUNT].temperature3 = lround(random(50, 60));
  SensorBuffer[SAMPLE_COUNT].humidity = random(0, 10);

  ytGraph(Graph, SAMPLE_COUNT, SensorBuffer[SAMPLE_COUNT].temperature1, TFT_CYAN, ox1, oy1);
  ytGraph(Graph, SAMPLE_COUNT, SensorBuffer[SAMPLE_COUNT].temperature2, TFT_PINK, ox2, oy2);
  ytGraph(Graph, SAMPLE_COUNT, SensorBuffer[SAMPLE_COUNT].temperature3, TFT_YELLOW, ox3, oy3);
  ytGraph(Graph, SAMPLE_COUNT, SensorBuffer[SAMPLE_COUNT].humidity, TFT_MAGENTA, ox4, oy4);

  xAxis.pushSprite(X_AXIS_LEFT_X, X_AXIS_UPPER_Y); // no Background color
  Graph.pushSprite(SPRITE_LEFT_X, SPRITE_UPPER_Y); // left upper position
}

void shiftBufferLeft()
{
  for (int8_t i = 1; i <= SAMPLE_COUNT; i++)
  {
    SensorBuffer[i - 1] = SensorBuffer[i];
  }
}

void simulateHistBufferWrite()
{
  static int graphVal = 0;
  static int delta = 1;
  for (uint8_t i = 0; i <= SAMPLE_COUNT; i++)
  {
    shiftBufferLeft(); // shift the whole array one position to the left

    // write new data at the right end of the array
    SensorBuffer[SAMPLE_COUNT].humidity = graphVal; // triangle demo data
    SensorBuffer[SAMPLE_COUNT].temperature1 = random(0, 200) / 10.0;
    SensorBuffer[SAMPLE_COUNT].temperature2 = random(200, 400) / 10.0;
    SensorBuffer[SAMPLE_COUNT].temperature3 = random(400, 600) / 10.0;

    // sabre tooth function
    graphVal += delta;
    if (graphVal >= GRAPH_HEIGHT)
      delta = -1; // ramp down value
    else if (graphVal <= 1)
      delta = +1; // ramp up value
  }
}

void drawUnbuffered()
{
  ytGraphDrawGridXaxis(Graph, xAxis, LastXGridLinePos); // refresh the grid

  // if using this function only, there is no need
  // to make the oy/ox variables global or static!
  oy1 = 0; // for each new drawing of unbuffered data, y axis starts at zero!
  oy2 = 0;
  oy3 = 0;
  oy4 = 0;

  ox1 = 0; // for each new drawing of unbuffered data, x axis starts at zero!
  ox2 = 0;
  ox3 = 0;
  ox4 = 0;

  // draw one full diagramm page for demo purposes
  // for endless drawing, repeat the whole function
  for (int8_t i = 0; i <= SAMPLE_COUNT; i++) // SAMPLE_COUNT = +1
  {
    ytGraph(Graph, i, lround(random(0, 200) / 10.0), TFT_CYAN, ox1, oy1);
    ytGraph(Graph, i, lround(random(200, 400) / 10.0), TFT_PINK, ox2, oy2);
    ytGraph(Graph, i, lround(random(400, 600) / 10.0), TFT_YELLOW, ox3, oy3);
    ytGraph(Graph, i, lround(random(100, 300) / 10.0), TFT_MAGENTA, ox4, oy4);
    delay(200);                                      // only to slow down drawing a little bit
    Graph.pushSprite(SPRITE_LEFT_X, SPRITE_UPPER_Y); // draw flicker free
  }
}

void setup()
{
#ifdef useM5STACK // only if using M5Stack
  M5.begin();
  M5.Power.begin();
#endif

  Serial.begin(115200);

  TFT.begin();
  TFT.setRotation(1); // landscape mode for M5Stack

  // splash screen
  TFT.fillScreen(TFT_WHITE);
  TFT.drawXBitmap((int)(TFT.width() / 2) - (int)(logoWidth / 2), 30, logo, logoWidth, logoHeight, TFT_BLACK);
  TFT.setTextDatum(MC_DATUM);
  TFT.setTextSize(2);
  TFT.setTextColor(TFT_BLACK, TFT_WHITE);
  TFT.drawString("ytGraph Demo", (int)(TFT.width() / 2), 120, 2);
  TFT.setTextSize(1);
  TFT.drawString("(c) Armin Pressler 2021", (int)(TFT.width() / 2), 155, 2);
  TFT.drawString(__FILENAME__, (int)(TFT.width() / 2), 170, 2);
  delay(3000);

  // info screen
  TFT.fillScreen(GRAPH_BGRND_COLOR);
  TFT.setTextColor(TFT_WHITE, GRAPH_BGRND_COLOR);
  TFT.setTextSize(2);
  TFT.setTextDatum(CL_DATUM);
  TFT.drawString("ytGraph functions", 40, 30, 2);
  TFT.setTextSize(1);
  TFT.setTextDatum(CL_DATUM);
  TFT.drawString(" - for time series data", 40, 80, 2);
  TFT.drawString(" - fully scalable", 40, 100, 2);
  TFT.drawString(" - flicker free scrolling", 40, 120, 2);
  TFT.drawString(" - up to 16 channels", 40, 140, 2);
  TFT.drawString(" - supports M5Stack & TFT_eSPI", 40, 160, 2);
  delay(3000);

  // prepare sprites for graph
  Graph.setColorDepth(4);                          // max 16 graph lines with different colors
  Graph.createSprite(SPRITE_WIDTH, SPRITE_HEIGTH); // height = width at M5Stack (landscape mode!)
  xAxis.setColorDepth(1);                          // save some kBytes, only 2 axis text colors available...
  xAxis.createSprite(X_AXIS_WIDTH, X_AXIS_HEIGTH);

  // draw demo with 4 channels and no buffer, e.g. could be used for life data
  // no scrolling - graph must be redrawed at the end of the last data point
  TFT.fillScreen(GRAPH_BGRND_COLOR);
  printDemoInfoText("draw 4 channels - no scrolling");
  delay(2000);
  ytGraphDrawYaxisFrame(TFT);                           // draw the y axis and the frame once
  ytGraphDrawGridXaxis(Graph, xAxis, LastXGridLinePos); // draw the grid
  drawUnbuffered();                                     // draw the lines of 4 channels
  printDemoInfoText("draw it one more time ;-)");            //
  delay(500);                                           //
  drawUnbuffered();                                     // draw the lines from the beginning a 2nd time for demo purposes
  delay(5000);

  // draw demo with historical (static) data stored in a buffer and
  // then draw data seamless and dynamic scrolled in the same graph
  TFT.fillScreen(GRAPH_BGRND_COLOR);
  printDemoInfoText("4 channels historical - no scrolling");
  delay(2000);
  ytGraphDrawYaxisFrame(TFT);                           // draw the y axis and the frame once
  ytGraphDrawGridXaxis(Graph, xAxis, LastXGridLinePos); // draw the grid

  memset(SensorBuffer, 0.0, sizeof(SensorBuffer)); // clear buffer array
  simulateHistBufferWrite();                       // prepare the historic demo data
  drawStaticGraphBuffer();                         // draw the stored data into the graph once

  printDemoInfoText("4 channels infinite - scrolling flicker free");
  delay(2000);
  // draw the time seris data into the scrolling graph
  for (;;) // simulate a endless loop
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) // non blocking delay
    {
      previousMillis = currentMillis;
      drawDynamicGraph();
    }
  }
}

void loop()
{
}
