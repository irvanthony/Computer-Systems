#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;
  //made variables of expressions so that they dont get evaluated in every iteration
  //ELIMINATING LOOP INEFFICIENCIES
  int w = (input -> width) - 1;
  int h = (input -> height) - 1;
  int div = filter -> getDivisor();
  int color1;
  int color2;
  int color3;
 //made an array to skip having to search values in every iteration
  int filterA[3][3];
 //looped through array to get rid of the two filter for loops within the three image for loops
 //REDUCING LOOP FUNCTIONS
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
        filterA[i][j] = filter -> get(i, j);
	}
  }
 //swapped for loops for better spatial locality
  for(int plane = 0; plane < 3; plane++) 
  {
	for(int row = 1; row < h; row++) 
    {
		for(int col = 1; col < w; col++) 
        {
	     color1 = color1 + (input -> color[plane][row-1][col-1] * filterA[0][0] );
		 color2 = color2 + (input -> color[plane][row-1][col] * filterA[0][1] );
		 color3 = color3 +(input -> color[plane][row-1][col+1] * filterA[0][2] );
		
		 color1 = color1 + (input -> color[plane][row][col-1] * filterA[1][0] );
		 color2 = color2 + (input -> color[plane][row][col] * filterA[1][1] );
		 color3 = color3 + (input -> color[plane][row][col+1] * filterA[1][2] );
		
		 color1 = color1 + (input -> color[plane][row+1][col-1] * filterA[2][0] );
		 color2 = color2 + (input -> color[plane][row+1][col] * filterA[2][1] );
		 color3 = color3 + (input -> color[plane][row+1][col+1] * filterA[2][2] );
		
         output -> color[plane][row][col] = color1 + color2 + color3;
         //made if statement to skip this step when the divisor is 1 since it makes no difference
         if ( div > 1)
         {
             output -> color[plane][row][col] = output -> color[plane][row][col] / div;
	     }
         //made if statement to only check when the <0 AND >255 is broken to skip two unecessary steps
         if ( output -> color[plane][row][col] > 0 && output -> color[plane][row][col] < 255)
         {
             
         }
         else if ( output -> color[plane][row][col]  > 255 )
         {
            output -> color[plane][row][col] = 255;
         }
         else if ( output -> color[plane][row][col]  < 0 )
         {
            output -> color[plane][row][col] = 0;
         }
      }
    }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}


/*

double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;
  //made variables of expressions so that they dont get evaluated in every iteration
  //ELIMINATING LOOP INEFFICIENCIES
  int w = (input -> width) - 1;
  int h = (input -> height) - 1;
  int color1;
  int color2;
  int color3;
  int c1;
  int c2;
  int c3;
  int col1;
  int col2;
  int col3;
  int div = filter -> getDivisor();
    
    
  int filterA [3][3];
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
        filterA[i][j] = filter -> get(i, j);
    }
  }
    
   for(int row = 1; row < h; row = row++) //swaped for better spatial locality
    {
        for(int col = 1; col < w; col++) 
        { 
           
            color1 = 0;
            color2 = 0;
            color3 = 0;
          
            /*
            for (int j = 0; j < filter -> getSize(); j++) 
               {
                  for (int i = 0; i < filter -> getSize(); i++) 
                  {	
                    color1 = color1 + (input->color[0][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                    color2 = color2 + (input->color[1][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                    color3 = color3 + (input->color[2][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                  }
               }
             
             
            color1 = color1 + (input->color[0][row - 1][col - 1] * filterA[0][0]);
            color1 = color1 + (input->color[0][row + 0][col - 1] * filterA[1][0]);
            color1 = color1 + (input->color[0][row + 1][col - 1] * filterA[2][0]);
            
            color2 = color2 + (input->color[0][row - 1][col] * filterA[0][1]);
            color2 = color2 + (input->color[0][row + 0][col] * filterA[1][1]);
            color2 = color2 + (input->color[0][row + 1][col] * filterA[2][1]);
            
            color3 = color3 + (input->color[0][row - 1][col + 1] * filterA[0][2]);
            color3 = color3 + (input->color[0][row + 0][col + 1] * filterA[1][2]);
            color3 = color3 + (input->color[0][row + 1][col + 1] * filterA[2][2]);
            
            c1 = c1 + (input->color[1][row - 1][col - 1] * filterA[0][0]);
            c1 = c1 + (input->color[1][row + 0][col - 1] * filterA[1][0]);
            c1 = c1 + (input->color[1][row + 1][col - 1] * filterA[2][0]);
            
            c2 = c2 + (input->color[1][row - 1][col] * filterA[0][1]);
            c2 = c2 + (input->color[1][row + 0][col] * filterA[1][1]);
            c2 = c2 + (input->color[1][row + 1][col] * filterA[2][1]);
            
            c3 = c3 + (input->color[1][row - 1][col + 1] * filterA[0][2]);
            c3 = c3 + (input->color[1][row + 0][col + 1] * filterA[1][2]);
            c3 = c3 + (input->color[1][row + 1][col + 1] * filterA[2][2]);
            
            col1 = col1 + (input->color[2][row - 1][col - 1] * filterA[0][0]);
            col1 = col1 + (input->color[2][row + 0][col - 1] * filterA[1][0]);
            col1 = col1 + (input->color[2][row + 1][col - 1] * filterA[2][0]);
            
            col2 = col2 + (input->color[2][row - 1][col] * filterA[0][1]);
            col2 = col2 + (input->color[2][row + 0][col] * filterA[1][1]);
            col2 = col2 + (input->color[2][row + 1][col] * filterA[2][1]);
            
            col3 = col3 + (input->color[2][row - 1][col + 1] * filterA[0][2]);
            col3 = col3 + (input->color[2][row + 0][col + 1] * filterA[1][2]);
            col3 = col3 + (input->color[2][row + 1][col + 1] * filterA[2][2]);
                               
            
            color1 = color1 + color2 + color3;
            c1 = c1 + c2 + c3;
            col1 = col1 + col2 + col3;
            
                  color1 = color1 / div;
                  c1 = c1 / div;
                  col1 = col1 / div;
                  if (color1 < 0)
                  {
                      color1 = 0;
                  }
                  if (color1 > 255)
                  {
                      color1 = 255;
                  }
                  if (c1 < 0)
                  {
                      c1 = 0;
                  }
                  if (c1 > 255)
                  {
                      c1 = 255;
                  }
                  if (col1 < 0)
                  {
                      col1 = 0;
                  }
                  if (col1 > 255)
                  {
                      col1 = 255;
                  }
              
              
              output -> color[0][row][col] = color1;
              output -> color[1][row][col] = c1;
              output -> color[2][row][col] = col1;
        }
   }
 

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}

/*

double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;


  //for(int col = 1; col < (input -> width) - 1; col = col + 1) {
    for(int row = 1; row < (input -> height) - 1 ; row = row + 1) 
    {
        for(int col = 1; col < (input -> width) - 1; col = col + 1) 
        { 
           
          int color1 = 0;
          int color2 = 0;
          int color3 = 0;
          
            for (int j = 0; j < filter -> getSize(); j++) 
               {
                  for (int i = 0; i < filter -> getSize(); i++) 
                  {	
                    color1 = color1 + (input->color[0][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                    color2 = color2 + (input->color[1][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                    color3 = color3 + (input->color[2][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                  }
               }
              
              color1 = color1 / filter -> getDivisor();
              if (color1 < 0)
              {
                  color1 = 0;
              }
              if (color1 > 255)
              {
                  color1 = 255;
              }
              color2 = color2 / filter -> getDivisor();
              if (color2 < 0)
              {
                  color2 = 0;
              }
              if (color2 > 255)
              {
                  color2 = 255;
              }
              color3 = color3 / filter -> getDivisor();
              if (color3 < 0)
              {
                  color3 = 0;
              }
              if (color3 > 255)
              {
                  color3 = 255;
              }
              
              output -> color[0][row][col] = color1;
              output -> color[1][row][col] = color2;
              output -> color[2][row][col] = color3;
              
              
              
              
          /*for(int plane = 0; plane < 3; plane++) 
          {
               int t = 0;
               output -> color[plane][row][col] = 0;
               for (int j = 0; j < filter -> getSize(); j++) 
               {
                  for (int i = 0; i < filter -> getSize(); i++) 
                  {	
                    output -> color[plane][row][col] = output -> color[plane][row][col]
                    + (input -> color[plane][row + i - 1][col + j - 1] * filter -> get(i, j) );
                    
                  }
                }
            output -> color[plane][row][col] = 	output -> color[plane][row][col] / filter -> getDivisor(); 
            
            if ( output -> color[plane][row][col]  < 0 ) {
                  output -> color[plane][row][col] = 0;
            }
            if ( output -> color[plane][row][col]  > 255 ) { 
            output -> color[plane][row][col] = 255;
            }
            
          
       }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}

*/ 

