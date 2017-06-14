struct ColorCoordinate
{
    ColorCoordinate()
    {

    }
    ColorCoordinate(const AppParameters &parameters, RGBColor _color, Vec2i _coord, int width, int height)
    {
        color = _color;
        coord = _coord;
        MakeFeatureVector(parameters, width, height);
    }
    RGBColor color;
    Vec2i coord;
    float features[5];

private:
    void MakeFeatureVector(const AppParameters &parameters, int width, int height)
    {
        features[0] = color.r / 255.0f;
        features[1] = color.g / 255.0f;
        features[2] = color.b / 255.0f;
        features[3] = coord.x/(float)width * parameters.spatialToColorScale;
        features[4] = coord.y/(float)height * parameters.spatialToColorScale;
    }
};

class SuperpixelExtractorPeriodic
{
public:
	Vector<ColorCoordinate> Extract(const AppParameters &parameters, const Bitmap &bmp);
};

struct Superpixel
{
    void Reset(const Bitmap &bmp, const Vec2i &seed);

    Vec2i MassCentroid() const;
    double AssignmentError(const Bitmap &bmp, const Vec2i &coord) const;
    
    __forceinline void AddCoord(const Vec2i &coord)
    {
        pixels.PushEnd(coord);
    }
    void ResetColor( const RGBColor &_color );
    void ComputeColor( const Bitmap &bmp );

    Vec3f color;
    Vector<Vec2i> pixels;
    Vec2i seed;
};



class SuperpixelExtractorSuperpixel
{
public:
    struct QueueEntry
    {
        double priority;
        Vec2i coord;
        UINT superpixelIndex;
    };

    Vector<ColorCoordinate> Extract(const AppParameters &parameters, const Bitmap &bmp);
	Vector<ColorCoordinate> Extract(const AppParameters &parameters, const Bitmap &bmp, Grid<UINT> &assignmentsOut);
    void Extract(const AppParameters &parameters, const Bitmap &bmp, Vector<Superpixel> &superpixelsOut, Grid<UINT> &assignmentsOut);

private:
    void InitializeSuperpixels(const AppParameters &parameters, const Bitmap &bmp);
    void AssignPixel(const AppParameters &parameters, const Bitmap &bmp, const Vec2i &coord, UINT clusterIndex);
    void GrowSuperpixels(const AppParameters &parameters, const Bitmap &bmp);
    void RecenterSuperpixels(const AppParameters &parameters, const Bitmap &bmp);
    
    static void DrawSuperpixelIDs(const Grid<UINT> &superpixelIDs, Bitmap &bmp);
    void DrawSuperpixelColors(const Bitmap &inputBmp, Bitmap &outputBmp);

    Vector<Superpixel> _superpixels;
    priority_queue<QueueEntry> _queue;
    Grid<UINT> _assignments;
    Vec2i _dimensions;
};

__forceinline bool operator < (const SuperpixelExtractorSuperpixel::QueueEntry &a, const SuperpixelExtractorSuperpixel::QueueEntry &b)
{
    return (a.priority < b.priority);
}
