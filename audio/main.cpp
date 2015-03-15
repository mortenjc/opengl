#include <fstream>
#include <iostream>
#include <string>
using namespace std;

#include <fourier.h>
#include <nr3.h>

//#define NFFT 32768
#define NFFT 128
int specidx=0;

class WavFileForIO
{
/*  FROM http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

private:
char* 	myPath;
int 	myChunkSize;
int		mySubChunk1Size;
short 	myFormat;
short 	myChannels;
int   	mySampleRate;
int   	myByteRate;
short 	myBlockAlign;
short 	myBitsPerSample;
int		myDataSize;
		
public:
char* 	myData;


char* getPath() // get/set for the Path property
{
	return myPath;
}
void setPath(char* newPath)
{
	myPath = new char[200];
	strcpy(myPath, newPath);
}

~WavFileForIO()
{
	delete myPath;
	myChunkSize = NULL;
	mySubChunk1Size = NULL;
	myFormat = NULL;
	myChannels = NULL;
	mySampleRate = NULL;
	myByteRate = NULL;
	myBlockAlign = NULL;
	myBitsPerSample = NULL;
	myDataSize = NULL;
}

WavFileForIO() // empty constructor
{
	myPath = new char[200];
}

WavFileForIO(char* tmpPath) // constructor takes a wav path
{
	myPath = new char[200];
	strcpy(myPath, tmpPath);
	read();
}

// read a wav file into this class
bool read()
{
	ifstream inFile( myPath, ios::in | ios::binary);

	inFile.seekg(4, ios::beg);
	inFile.read( (char*) &myChunkSize, 4 ); // read the ChunkSize

	inFile.seekg(16, ios::beg);
	inFile.read( (char*) &mySubChunk1Size, 4 ); // read the SubChunk1Size

	//inFile.seekg(20, ios::beg);
	inFile.read( (char*) &myFormat, sizeof(short) ); // read the file format.  This should be 1 for PCM

	//inFile.seekg(22, ios::beg);
	inFile.read( (char*) &myChannels, sizeof(short) ); // read the # of channels (1 or 2)

	//inFile.seekg(24, ios::beg);
	inFile.read( (char*) &mySampleRate, sizeof(int) ); // read the samplerate

	//inFile.seekg(28, ios::beg);
	inFile.read( (char*) &myByteRate, sizeof(int) ); // read the byterate

	//inFile.seekg(32, ios::beg);
	inFile.read( (char*) &myBlockAlign, sizeof(short) ); // read the blockalign

	//inFile.seekg(34, ios::beg);
	inFile.read( (char*) &myBitsPerSample, sizeof(short) ); // read the bitspersample

	inFile.seekg(40, ios::beg);
	inFile.read( (char*) &myDataSize, sizeof(int) ); // read the size of the data

	// read the data chunk
	myData = new char[myDataSize];
	inFile.seekg(44, ios::beg);
	inFile.read(myData, myDataSize);

	inFile.close(); // close the input file

	return true; // this should probably be something more descriptive
}

#define SPECSLOTS 20
#define FREQSLOTS 24

NRmatrix <double> spectrum(SPECSLOTS, FREQSLOTS); // Holds spectrum binned in FREQSLOTS

void add_spectrum(VecDoub_IO &data)
{
	printf("Spectrum index %d\n", specidx);
	for (int i = 0; i< FREQSLOTS; i++)
		spectrum[specidx][i]= 0.0;
	
	for (int i=0;i <NFFT; i+=1)
	{
		//printf("add index %d  data %.0f\n", i, data[i]);
		spectrum[specidx][(int)i*FREQSLOTS/NFFT] += data[i];
	}
	for (int i=0;i < FREQSLOTS; i++)
		printf("freqbin %12d  freq %12.0f  value %12.0f\n", i, (double)i*mySampleRate/FREQSLOTS/2 , (spectrum[specidx][i]/NFFT)*(spectrum[specidx][i]/NFFT));

	specidx = (++specidx)%SPECSLOTS; //Point to next in circular buffer
}


void print()
{
	VecDoub_IO data(NFFT);
	double dt = 1.0/mySampleRate;
	int once=0;

	int mult = myBitsPerSample>>3;  // 1, 2, ...
	
	for (int i=0; i<myDataSize; i+=mult) // Run through entire wav file, datasize in bytes samples in shorts
	{
		if ( ((i/mult)%(mySampleRate)) == 0 ) 		// Do an FFT every second
		{
			printf("i: %d   time: %f  ", i/mult, i/mult*dt);
			printf("sampling %f seconds\n", NFFT*dt);
			
			if (once==0) // For test purposes only do this once
			{
				for (int j=0;j<NFFT;j++)
				{
					if (mult == 1)
					   data[j] = (double)myData[i+j*mult];
					else if (mult == 2) 
					   data[j] = (double)(myData[i+j*mult] + 256*myData[i+j*mult+1]);
					else
					{
						printf("Unsupported datasize %d\n", mult);
						exit(0);
					//data[j] = 200*sin(j/5.0) + 100*sin(j*5.0);
					//printf("sample %4d  time %f   value %.0f\n", j, j*dt, data[j]);
					}
				}
				realft(data, 1);
				add_spectrum(data);
				
				for (int j=0;j<NFFT;j++)
				{
					//printf("sample %4d  freq %7d   value %.0f\n", j, (int)mySampleRate*j/NFFT/2, data[j]);
				}
				//once=1;
			}
		}
	}
}


bool save() // write out the wav file
{
	fstream myFile (myPath, ios::out | ios::binary);

	// write the wav file per the wav file format
	myFile.seekp (0, ios::beg); 
	myFile.write ("RIFF", 4);
	myFile.write ((char*) &myChunkSize, 4);
	myFile.write ("WAVE", 4);
	myFile.write ("fmt ", 4);
	myFile.write ((char*) &mySubChunk1Size, 4);
	myFile.write ((char*) &myFormat, 2);
	myFile.write ((char*) &myChannels, 2);
	myFile.write ((char*) &mySampleRate, 4);
	myFile.write ((char*) &myByteRate, 4);
	myFile.write ((char*) &myBlockAlign, 2);
	myFile.write ((char*) &myBitsPerSample, 2);
	myFile.write ("data", 4);
	myFile.write ((char*) &myDataSize, 4);
	myFile.write (myData, myDataSize);

	return true;
}


char *getSummary() // return a printable summary of the wav file
{
	char *summary = new char[250];
	sprintf(summary, " Format: %d\n Channels: %d\n SampleRate: %d\n ByteRate: %d\n BlockAlign: %d\n BitsPerSample: %d\n DataSize: %d\n", myFormat, myChannels, mySampleRate, myByteRate, myBlockAlign, myBitsPerSample, myDataSize);
	return summary;
}
};


int main( int argc, char *argv[] )
{
	char *path = new char[50];
	strcpy_s(path, 50, "crest_16.wav");
	//strcpy_s(path, 50, "checked.wav"); 
	WavFileForIO *myWav = new WavFileForIO(path);

	char *summary = myWav->getSummary();
	printf("Summary:\n%s", summary);	

	myWav->print();

	delete summary;
	delete path;
	delete myWav;

	return 0;
}


// write the summary back out
	//strcpy_s(path, 50, "testout.wav");
	//myWav->setPath(path);
	//myWav->save();
