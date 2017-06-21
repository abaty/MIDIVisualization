#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <stdlib.h>
#include <vector>
//#include <TH1D.h>
//#include <TFile.h>

enum class WavChunks {
    RiffHeader = 0x46464952,
    WavRiff = 0x54651475,
    Format = 0x020746d66,
    LabeledText = 0x478747C6,
    Instrumentation = 0x478747C6,
    Sample = 0x6C706D73,
    Fact = 0x47361666,
    Data = 0x61746164,
    Junk = 0x4b4e554a,
};

enum class WavFormat {
    PulseCodeModulation = 0x01,
    IEEEFloatingPoint = 0x03,
    ALaw = 0x06,
    MuLaw = 0x07,
    IMAADPCM = 0x11,
    YamahaITUG723ADPCM = 0x16,
    GSM610 = 0x31,
    ITUG721ADPCM = 0x40,
    MPEG = 0x50,
    Extensible = 0xFFFE
};

void readWav(std::pair< std::vector< int >, std::pair < std::vector < short >, std::vector < short > > >& wavInfo){
  std::string file = "testFiles/Dvorak/DvorakCzechSuite1.wav";
  //std::string file = inputFile.append(".wav");
  FILE * pFile;
  pFile = fopen ( file.c_str() , "rb" );

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  int  lSize = ftell (pFile);
  rewind (pFile);

  // allocate memory to contain the whole file:
  char * buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  bool datachunk = false;
  int chunkid, formatsize, channelcount, samplerate, bitspersecond, memsize, riffstyle, datasize, headerid;
  short format, channels, formatblockalign, bitdepth;

  while ( !datachunk ) {
    //chunkid = reader.ReadInt32( );
    fread(&chunkid,sizeof(int),1,pFile);

    switch ( (WavChunks)chunkid ) {
    case WavChunks::Format:
        fread(&formatsize,sizeof(int),1,pFile);// = reader.ReadInt32( );
        fread(&format,sizeof(short),1,pFile);// = (WavFormat)reader.ReadInt16( );
        fread(&channels,sizeof(short),1,pFile);// = (Channels)reader.ReadInt16( );
        channelcount = (int)channels;
        fread(&samplerate,sizeof(int),1,pFile);// = reader.ReadInt32( );
        fread(&bitspersecond,sizeof(int),1,pFile);// = reader.ReadInt32( );
        fread(&formatblockalign,sizeof(short),1,pFile);// = reader.ReadInt16( );
        fread(&bitdepth,sizeof(short),1,pFile);// = reader.ReadInt16( );
        if ( formatsize == 18 ) {
            int extradata;
            fread(&extradata,sizeof(int),1,pFile);// = reader.ReadInt16( );
            fseek(pFile,extradata,SEEK_CUR);
        }
        break;
    case WavChunks::RiffHeader:
        headerid = chunkid;
        fread(&memsize,sizeof(int),1,pFile);// = reader.ReadInt32( );
        fread(&riffstyle,sizeof(int),1,pFile);// = reader.ReadInt32( );
        break;
    case WavChunks::Data:
        datachunk = true;
        fread(&datasize,sizeof(int),1,pFile);// = reader.ReadInt32( );
        break;
    default:
        int skipsize;// = reader.ReadInt32( );
        fread(&skipsize,sizeof(int),1,pFile);// = reader.ReadInt16( );
        fseek(pFile,skipsize,SEEK_CUR);
        break;
    }
  }
  std::vector< int > metaInfo(0);
  wavInfo.first = metaInfo;
  std::cout << "Channels: " << channelcount << std::endl;
  wavInfo.first.push_back(channelcount);
  std::cout << "Sample Rate: " << samplerate << std::endl;
  wavInfo.first.push_back(samplerate);
  std::cout << "Sample size: " << formatblockalign << std::endl;
  wavInfo.first.push_back(formatblockalign);
  std::cout << "Data size: " << datasize << std::endl;
  wavInfo.first.push_back(datasize);
  std::cout << "bit depth: " << bitdepth << std::endl;
  wavInfo.first.push_back(bitdepth);

  short dat = 0;
  int t0 = 0;
  int isDone = false;
  std::vector< short > channel1;
  std::vector< short > channel2;

  /*TFile * f;
  TH1D * h;
  if (isRoot) {
	  f = TFile::Open("out.root", "recreate");
	  h = new TH1D("h", "h", 100000, 0, 100000);
  }*/
  int i = 0;
  std::cout << datasize << std::endl;
  while(true){
	if (i % 50000 == 0) std::cout << i << std::endl;
	i += 2*fread(&dat,2,1,pFile);
	channel1.push_back(dat);
	if (i >= datasize) break;
	//if(isRoot) h->Fill(i, dat);
    i += 2*fread(&dat,2,1,pFile);
	channel2.push_back(dat);
	if (i >= datasize) break;
  }
  wavInfo.second = std::pair< std::vector< short > , std::vector< short > >(channel1,channel2);
  std::cout << "Drawing" << std::endl;
  /*if (isRoot) {
	  h->Write();
	  f->Close();
  }*/
  fclose(pFile);
}

