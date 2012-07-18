#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <iostream>

#include <CImg.h>

using namespace std;
using namespace cimg_library;

// Description of a blob
struct Blob {
  Blob() : area(0) {}  
  union {
    unsigned int area; ///< Area (moment 00).
    unsigned int m00; ///< Moment 00 (area).
  };
    
  unsigned int minx; ///< X min.
  unsigned int maxx; ///< X max.
  unsigned int miny; ///< Y min.
  unsigned int maxy; ///< Y max.
    
  pair<float, float> centroid; ///< Centroid.
    
  double m10; ///< Moment 10.
  double m01; ///< Moment 01.

  vector<char> contour; ///< Contour as a chain code.
  // x and y coordinates of pixels of the blob
  vector<unsigned> x, y; 
  // x and y coordinates of the blob's border
  vector<unsigned> border_x, border_y;
};

//! Detection of regions (blobs) on an image and calculating blob's characteristics
/**
  \param  labeled_slice Labeled image, result of CImg<> label method
  \result Map of blobs
**/
map <unsigned, Blob> FindBlobs(const CImg<> &labeled_slice) {
  map <unsigned, Blob> blobs;
  // Area and coordinates of each blob
  cimg_forXY(labeled_slice, x, y) {
    unsigned val = labeled_slice(x,y); 
    blobs[val].area++;
    blobs[val].x.push_back(x);
    blobs[val].y.push_back(y);
  }
  // m01 and m10 moments
  map<unsigned, Blob>::iterator it;
  for (it = blobs.begin(); it != blobs.end(); ++it) {
    vector<unsigned> coords = it->second.x;
    it->second.minx = *(std::min_element(coords.begin(), coords.end()));
    it->second.maxx = *(std::max_element(coords.begin(), coords.end()));
    it->second.m10 = std::accumulate(coords.begin(), coords.end(), 0);
    coords = it->second.y;
    it->second.miny = *(std::min_element(coords.begin(), coords.end()));
    it->second.maxy = *(std::max_element(coords.begin(), coords.end()));
    it->second.m01 = std::accumulate(coords.begin(), coords.end(), 0);
    it->second.centroid.first = it->second.m10 / it->second.m00;
    it->second.centroid.second = it->second.m01 / it->second.m00;
  }

  // Border tracing, 4-connectivity
  /*
            1
            |
        2 --x-- 0
            |
            3    
  */
  int direction_ways[4][2] = {{1, 0}, {0, -1}, {-1, 0}, {0, 1}};
  unsigned im_w = labeled_slice.width(), im_h = labeled_slice.height();

  for (it = blobs.begin(); it != blobs.end(); ++it) {
    /*
      Search the image from top left until a pixel of new region is found;
      this pixel P0 is the starting pixel of the region border.
    */

//cout << endl << endl << "Index #" << it->first;

    unsigned current_x = it->second.x[0],
             current_y = it->second.y[0];

//cout << endl << "(" << current_x << ", " << current_y << ") ";

    it->second.border_x.push_back(current_x);
    it->second.border_y.push_back(current_y);

    // Border of single-pixel region
    if (1 == it->second.area) return blobs;
    /*
      Declare a variable which stores a direction of the previous move along the border
      from the previous border element to the current border element
    */
    unsigned direction = 0; //initial value if the border is detected in 4-connectivity
    /*
      Search the 3x3 neighborhood of the current pixel in a anti-clockwise direction,
      beginning the neighborhood search at the pixel  positioned in the direction
      (a) 4-connectivity
          (dir+3) mod 4
      (b) 8-connectivity
          (dir+7) mod 8 if dir is even
          (dir+6) mod 8 if dir is odd
      The first pixel found with the same value as the current pixel is a new boundary element Pn.
      If the current boundary element Pn is equal to the second border element P1 and if the previous 
      border element P(n-1) is equal to P0, stop. Otherwise repeat searching.
    */
    unsigned length = 1;
    while (true) {
      direction = (direction + 3) % 4;
      unsigned dir = direction;
      int  new_x = current_x + direction_ways[dir][0],
           new_y = current_y + direction_ways[dir][1];
      // Border neighbour's effect. If current pixel not in the image will not consider it
      int curr_pos_value = ((0 > new_x) || (0 > new_y) || (im_w <= new_x) || (im_h <= new_y))? -1 : labeled_slice(new_x, new_y);
      while (curr_pos_value != it->first) {
        dir = (dir+1) % 4;
        new_x = current_x + direction_ways[dir][0];
        new_y = current_y + direction_ways[dir][1];
        if ((0 > new_x) || (0 > new_y) || (im_w <= new_x) || (im_h <= new_y)) continue;
        curr_pos_value = labeled_slice(new_x, new_y);
      } // while (curr_pos_value != it->first)
      if ( (length > 1) &&  
            (new_x == it->second.border_x[1]) && (new_y == it->second.border_y[1]) &&
            current_x == it->second.border_x[0] && (current_y == it->second.border_y[0]))  {
        break;
      }
      current_x = new_x;
      current_y = new_y;
//cout << "(" << current_x << ", " << current_y << ") ";
      it->second.border_x.push_back(current_x);
      it->second.border_y.push_back(current_y);
      ++length;
      direction = dir;
    } // while (true) 
  }
  return blobs;
}

/* Returns the amount of milliseconds elapsed since the UNIX epoch. Works on both
 * windows and linux. 
 http://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
 */
long long GetTimeMs64() {
#ifdef WIN32
 /* Windows */
 FILETIME ft;
 LARGE_INTEGER li;
 /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
  * to a LARGE_INTEGER structure. */
 GetSystemTimeAsFileTime(&ft);
 li.LowPart = ft.dwLowDateTime;
 li.HighPart = ft.dwHighDateTime;
 unsigned long long ret = li.QuadPart;
 ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
 ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */
 return ret;
#else
 /* Linux */
 struct timeval tv;
 gettimeofday(&tv, NULL);
 uint64 ret = tv.tv_usec;
 /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
 ret /= 1000;
 /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
 ret += (tv.tv_sec * 1000);
 return ret;
#endif
}

int main(int argc, char **argv) {
  CImg<> img;
  if (argc > 1) {
    img.load(argv[1]);
  } else {
    cout << "Please define an image as command-line parameter.";
    return -1;
  }
  CImg<> img_binary = img.get_threshold(1),
         img_labeled =  img_binary.get_label(false); // 4-connectivity
  (img_binary, img_labeled).display();

  long long curr_time;
  curr_time = GetTimeMs64();
  map <unsigned, Blob> blobs = FindBlobs(img_labeled);
  cout << endl << "Blob calculating time (ms): " << double(GetTimeMs64()- curr_time) << endl;

  // Visualizing blob's info
  CImg<> img_vis = img, color(3);
  map<unsigned, Blob>::iterator it;
  for (it = blobs.begin(); it != blobs.end(); ++it) {
    cimg_forX(color,i) color[i] = (unsigned char)(128+(std::rand()%127));
    vector<unsigned>  border_x = it->second.border_x,
                      border_y = it->second.border_y;
    unsigned border_length = border_x.size();
    for (size_t i = 0; i < border_length; ++i) {
      img_vis.draw_circle(border_x[i], border_y[i], 2, color.data());
    }
  }
  (img_binary, img_labeled, img_vis).display();
  return 0;
}