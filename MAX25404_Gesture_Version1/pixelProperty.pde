class PixelProperty {
  int _len;
  float _Xoffset, _Yoffset;
  float _Colx, _Rowy;
  int _colSector;
  int _colIndex;
  
  final int[] PixelSectors = {1,1,1,1,1,2,2,2,2,2,1,1,1,1,0,0,2,2,2,2,1,1,1,0,0,0,0,2,2,2,4,4,4,0,0,0,0,3,3,3,4,4,4,4,0,0,3,3,3,3,4,4,4,4,4,3,3,3,3,3};
  // define colour properties for each sector
  final int[][] SectorProperties = {
                                    {255, 75, 0} /*sector 0*/, 
                                    {0, 75, 255} /*sector 1*/, 
                                    {0, 175, 175} /*sector 2*/, 
                                    {0, 255, 75} /*sector 3*/, 
                                    {175, 175, 0} /*sector 4*/
                                  };

  
  // Contructor
  PixelProperty(int tempL, int tempXoffset, int tempYoffset, int tempColx, int tempRowy, int temp_colSector, int temp_colIndex) {
    _len = tempL;
    _Xoffset = tempXoffset;
    _Yoffset = tempYoffset;
    _Colx = (tempColx*_len) + _Xoffset;
    _Rowy = (tempRowy*_len) + _Yoffset;
    colorMode(RGB);
    //strokeWeight(2);
    fill(25, 25, 25, 50);
    //noStroke();
    _colSector = temp_colSector;
    _colIndex = temp_colIndex;
  }
  
  // Custom method for updating the variables
  void update(int PixNo, float Alpha) {
    float _R = SectorProperties[PixelSectors[PixNo]][0];
    float _G = SectorProperties[PixelSectors[PixNo]][1];
    float _B = SectorProperties[PixelSectors[PixNo]][2];
    fill(_R, _G, _B, Alpha);
  }
  
  // Custom method for drawing the object
  void display() {
    square(_Colx, _Rowy, _len);
  }
  
  void displayVar(int PixVar) {
    stroke(50);              // Now set a grey color for points
    // using height as proxy measurement for width (to left align)
    line(_colIndex*10+50, height/2, _colIndex*10+50, height/2 + map(PixVar, -100, 100, -height/2, height/2));
  }
  
  int getCentrePix(int PixNo) {
    return PixelSectors[PixNo];
  }

  void displayPoint(int PixVal) {
    if (_colSector == 1) {
      stroke(210, 52, 0);
      strokeWeight(4);
    }
    else if (_colSector == 2) {
      stroke(210, 210, 0);
      strokeWeight(5);
    }
    else if (_colSector == 3) {
      stroke(0, 210, 52);
      strokeWeight(4);
    }
    point(_colIndex*10+50, map(PixVal, -32768, 32767, 0, 600));
  }
}
