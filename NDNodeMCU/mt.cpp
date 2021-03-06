#include "mt.h"

DisplayParser::DisplayParser(Display& display)
: m_display(display)
{}

bool DisplayParser::parse(const uint8_t byteIn)
{
    static const int headerSize = sizeof(PacketHeader);
    static uint8_t parsingMode = MAGICWORD;

    static uint32_t magicWord;

    static uint8_t header[headerSize];
    static PacketHeader* pHeader = (PacketHeader*)header;

    static int payloadParseIndex = 0;
    static int index = 0;

    bool tempBool = false;

    switch (parsingMode)
    {
    case MAGICWORD:
        magicWord = magicWord << 8 | byteIn;
        if (magicWord == 0xDEC0DED)
        {
            //Serial.printf("MAGICWORD\n");
            magicWord = 0;
            parsingMode = REST_OF_HEADER;
            index = 3;
        }
        break;

    case REST_OF_HEADER:
        
        //Serial.printf("Appending to header. Index = %d\n", index);
        header[index] = byteIn;
        if (index == headerSize - 1)
        {
            //Serial.printf("Done reading header\n");
            //Serial.printf("Packet type: %d\n", pHeader->packetType);
            //Serial.printf("Packet size: %d\n", pHeader->packetSize);
            parsingMode = PAYLOAD;
            payloadParseIndex = 0;
        }
        break;
    
    case PAYLOAD:

        //payloadArr[payloadParseIndex++] = byteIn;

        bool done = false;
        switch (pHeader->packetType)
        {
        case MODE:
            //Serial.printf("Unpacking MODE\n");
            done = parseMode(payloadParseIndex++, pHeader->packetSize, byteIn);
            break;

        case COMMAND:
            //Serial.printf("Unpacking COMMAND\n");
            done = parseCommand(payloadParseIndex++, pHeader->packetSize, byteIn);
            break;

        case DISPLAY_INPUT:
            //Serial.printf("Unpacking DISPLAY_INPUT\n");
            done = parseDisplay(payloadParseIndex++, pHeader->packetSize, byteIn);
            break;
        }
        if (done)
        {
            // We are done parsing payload
            parsingMode = MAGICWORD;
            payloadParseIndex = 0;
            index = 0;
        }
        break;
    }
    index++;

    return tempBool;
}

bool DisplayParser::parseMode(const int index, const int packetSize, const uint8_t byteIn)
{
    static PacketMode modePkt;

    modePkt.mode = byteIn;

    //Serial.printf("Mode received. Mode: %d\n", modePkt.mode);

    m_display.setMode((Display::Mode)modePkt.mode);

    return true;
}

bool DisplayParser::parseCommand(const int index, const int packetSize, const uint8_t byteIn)
{
    static PacketCommand commandPkt;

    switch (index)
    {
    case 0:
        commandPkt.command = byteIn;
        break;
    case 1:
        commandPkt.value = byteIn;
        //Serial.printf("Command received: %d, %d\n", commandPkt.command, commandPkt.value);

        actuateCommand((Command)commandPkt.command, commandPkt.value);
        return true;
        break;
    }
    return false;
}

bool DisplayParser::parseDisplay(const int index, const int packetSize, const uint8_t byteIn)
{
    static PacketDisplayUpdate displayPkt;
    static uint8_t pixelArr[4];
    //static Display::Pixel pixel;
    static Display::Pixel* pPixel = (Display::Pixel*)pixelArr;

    //Serial.printf("DisplayPart Index: %d, pixels: %d\n", index, displayPkt.pixels.size());
    switch (index)
    {
    case 0:
        displayPkt.typeOfUpdate = byteIn;
        displayPkt.pixels.clear();
        displayPkt.numOfPixels = 0;
        //Serial.printf("Rinsing pixel content. Num of pixels: %d\n", displayPkt.pixels.size());
        break;
    case 1:
    case 2:
        //Serial.printf("Prev numOfPixels: %d, index: %d\n", displayPkt.numOfPixels, index);
        displayPkt.numOfPixels = displayPkt.numOfPixels << 1 | byteIn;
        //Serial.printf("New  numOfPixels: %d\n", displayPkt.numOfPixels);
        break;
    default:
        //Serial.printf("Processing pixel...\n");
        uint8_t innerIndex = (index - 3) % 4;
        pixelArr[innerIndex] = byteIn;

        if (innerIndex == 3)
        {
            displayPkt.pixels.push_back(*pPixel);
            //Serial.printf("Pushing new pixel\n");
        }
        if (displayPkt.pixels.size() == displayPkt.numOfPixels)
        {
            //Serial.printf("Display received. Size %d\n", displayPkt.pixels.size());
            //Serial.printf("First pixel with index %d: %d, %d, %d\n", displayPkt.pixels[0].index, displayPkt.pixels[0].r, displayPkt.pixels[0].g, displayPkt.pixels[0].b);
            actuateDisplay((DisplayUpdate)displayPkt.typeOfUpdate, displayPkt.pixels);
            return true;
        }
        break;
    }

    return false;
}

void DisplayParser::actuateCommand(Command command, const uint8_t value)
{
    switch (command)
    {
    case CLEAR:
        m_display.clear();
        break;
    case BRIGHTNESS:
        m_display.setBrightness(value);
        break;
    case TEST:
        m_display.test();
        break;
    }
}

void DisplayParser::actuateDisplay(DisplayUpdate update, const std::vector<Display::Pixel>& pixels)
{
    m_display.setPixels(pixels, Display::FOREGROUND, Display::PARTIAL);
}