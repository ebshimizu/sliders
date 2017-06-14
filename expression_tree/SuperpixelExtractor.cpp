#include "Main.h"

Vector<ColorCoordinate> SuperpixelExtractorPeriodic::Extract(const AppParameters &parameters, const Bitmap &bmp)
{
    Vector<ColorCoordinate> result;
    for(UINT y = 0; y < bmp.Height(); y += parameters.periodicBasisCount)
    {
        for(UINT x = 0; x < bmp.Width(); x += parameters.periodicBasisCount)
        {
            ColorCoordinate coord(parameters, bmp[y][x], Vec2i(x, y), bmp.Width(), bmp.Height());
            result.PushEnd(coord);
        }
    }
    return result;
}

void Superpixel::Reset(const Bitmap &bmp, const Vec2i &_seed)
{
    pixels.FreeMemory();
    seed = _seed;
    AddCoord(seed);
}

Vec2i Superpixel::MassCentroid() const
{
    Vec2f center = Vec2f::Origin;
    for(const Vec2i &p : pixels) center += Vec2f(p);
    center /= float(pixels.Length());
    
    Vec2i bestCoord = pixels[0];
    float bestDistSq = 100000000.0f;
    for(const Vec2i &p : pixels)
    {
        float curDistSq = Vec2f::DistSq(Vec2f(p), center);
        if(curDistSq < bestDistSq)
        {
            bestDistSq = curDistSq;
            bestCoord = p;
        }
    }
    return bestCoord;
}

double Superpixel::AssignmentError(const Bitmap &bmp, const Vec2i &coord) const
{
    return Vec3f::DistSq(color, Vec3f(bmp[coord.y][coord.x]));
}

void Superpixel::ResetColor( const RGBColor &_color )
{
    color = Vec3f(_color);
}

void Superpixel::ComputeColor( const Bitmap &bmp )
{
    color = Vec3f::Origin;
    for(Vec2i p : pixels) color += Vec3f(bmp[p.y][p.x]);
    color /= (float)pixels.Length();
}

Vector<ColorCoordinate> SuperpixelExtractorSuperpixel::Extract(const AppParameters &parameters, const Bitmap &bmp)
{
	Grid<UINT> assignmentsOut;
	return Extract(parameters, bmp, assignmentsOut);
}

Vector<ColorCoordinate> SuperpixelExtractorSuperpixel::Extract(const AppParameters &parameters, const Bitmap &bmp, Grid<UINT> &assignmentsOut)
{
    Vector<Superpixel> superpixelsOut;
    Extract(parameters, bmp, superpixelsOut, assignmentsOut);

    Vector<ColorCoordinate> result;
    for(const Superpixel &p : superpixelsOut)
    {
        result.PushEnd(ColorCoordinate(parameters, RGBColor(p.color), p.seed, bmp.Width(), bmp.Height()));
    }
    return result;
}

void SuperpixelExtractorSuperpixel::Extract(const AppParameters &parameters, const Bitmap &bmp, Vector<Superpixel> &superpixelsOut, Grid<UINT> &assignmentsOut)
{
    ComponentTimer timer( "Segmenting bitmap, " + String(bmp.Width()) + "x" + String(bmp.Height()) );
    
    _dimensions = Vec2i(bmp.Width(), bmp.Height());
    _assignments.Allocate(_dimensions.y, _dimensions.x);
    _superpixels.Allocate(parameters.superpixelCount);

    InitializeSuperpixels(parameters, bmp);

    const UINT iterationCount = parameters.superpixelIterations;
    for(UINT iterationIndex = 0; iterationIndex < iterationCount; iterationIndex++)
    {
        Console::WriteLine("Starting superpixel iteration " + String(iterationIndex));
        //ComponentTimer timer( "Iteration " + String(iterationIndex) );
        GrowSuperpixels(parameters, bmp);

        const bool dumpIntermediateResults = true;
        if(dumpIntermediateResults)
        {
            Bitmap clusterBmp0, clusterBmp1;
            DrawSuperpixelIDs(_assignments, clusterBmp0);
            DrawSuperpixelColors(bmp, clusterBmp1);
            clusterBmp0.SavePNG("clustersIteration" + String(iterationIndex) + ".png");
            clusterBmp1.SavePNG("colorsIteration" + String(iterationIndex) + ".png");
        }

        RecenterSuperpixels(parameters, bmp);
    }

    GrowSuperpixels(parameters, bmp);
    for(Superpixel &p : _superpixels)
    {
        p.ComputeColor(bmp);
    }

    superpixelsOut = _superpixels;
    assignmentsOut = _assignments;

}

void SuperpixelExtractorSuperpixel::InitializeSuperpixels(const AppParameters &parameters, const Bitmap &bmp)
{
    Vector<Vec2i> seeds;
    for(Superpixel &p : _superpixels)
    {
        Vec2i randomSeed(rand() % _dimensions.x, rand() % _dimensions.y);
        while(seeds.Contains(randomSeed))
        {
            randomSeed = Vec2i(rand() % _dimensions.x, rand() % _dimensions.y);
        }
        
        p.ResetColor(bmp[randomSeed.y][randomSeed.x]);
        p.Reset(bmp, randomSeed);
        seeds.PushEnd(randomSeed);
    }
}

void SuperpixelExtractorSuperpixel::AssignPixel(const AppParameters &parameters, const Bitmap &bmp, const Vec2i &coord, UINT superpixelIndex)
{
    if(_assignments(coord.y, coord.x) != 0xFFFFFFFF)
    {
        return;
    }
    _assignments(coord.y, coord.x) = superpixelIndex;
    _superpixels[superpixelIndex].AddCoord(coord);

    const UINT neighborCount = 4;
    const UINT XOffsets[neighborCount] = {-1, 1, 0, 0};
    const UINT YOffsets[neighborCount] = {0, 0, -1, 1};
    for(UINT neighborIndex = 0; neighborIndex < neighborCount; neighborIndex++)
    {
        Vec2i finalCoord(coord.x + XOffsets[neighborIndex], coord.y + YOffsets[neighborIndex]);
        if(_assignments.ValidCoordinates(finalCoord.y, finalCoord.x) && _assignments(finalCoord.y, finalCoord.x) == 0xFFFFFFFF)
        {
            QueueEntry newEntry;
            newEntry.superpixelIndex = superpixelIndex;
            newEntry.coord = finalCoord;
            newEntry.priority = 1.0 - _superpixels[superpixelIndex].AssignmentError(bmp, finalCoord);
            _queue.push(newEntry);
        }
    }
}

void SuperpixelExtractorSuperpixel::GrowSuperpixels(const AppParameters &parameters, const Bitmap &bmp)
{
    _assignments.Clear(0xFFFFFFFF);

    //
    // Insert all seeds
    //
    for(UINT superpixelIndex = 0; superpixelIndex < _superpixels.Length(); superpixelIndex++)
    {
        AssignPixel(parameters, bmp, _superpixels[superpixelIndex].seed, superpixelIndex);
    }

    while(!_queue.empty())
    {
        QueueEntry curEntry = _queue.top();
        _queue.pop();
        AssignPixel(parameters, bmp, curEntry.coord, curEntry.superpixelIndex);
    }
}

void SuperpixelExtractorSuperpixel::RecenterSuperpixels(const AppParameters &parameters, const Bitmap &bmp)
{
    const UINT clusterSizeCutoff = 10;

    UINT teleportCount = 0;
    Vector<Vec2i> seeds;
    for(Superpixel &p : _superpixels)
    {
        Vec2i newSeed;
        if(p.pixels.Length() < clusterSizeCutoff)
        {
            newSeed = Vec2i(rand() % _dimensions.x, rand() % _dimensions.y);
            while(seeds.Contains(newSeed))
            {
                newSeed = Vec2i(rand() % _dimensions.x, rand() % _dimensions.y);
            }
            teleportCount++;
            p.ResetColor(bmp[newSeed.y][newSeed.x]);
        }
        else
        {
            newSeed = p.MassCentroid();
            p.ComputeColor(bmp);
        }
        p.Reset(bmp, newSeed);
        seeds.PushEnd(newSeed);
    }
}

void SuperpixelExtractorSuperpixel::DrawSuperpixelIDs(const Grid<UINT> &superpixelIDs, Bitmap &bmp)
{
    const UINT clusterCount = superpixelIDs.MaxValue() + 1;
    Vector<RGBColor> colors(clusterCount);
    for(RGBColor &c : colors) c = RGBColor::RandomColor();
    //ColorGenerator::Generate(colors);
    bmp.Allocate(superpixelIDs.Cols(), superpixelIDs.Rows());
    for(UINT y = 0; y < bmp.Height(); y++)
    {
        for(UINT x = 0; x < bmp.Width(); x++)
        {
            bmp[y][x] = colors[superpixelIDs(y, x)];
        }
    }
}

void SuperpixelExtractorSuperpixel::DrawSuperpixelColors(const Bitmap &inputBmp, Bitmap &outputBmp)
{
    outputBmp.Allocate(_assignments.Cols(), _assignments.Rows());
    for(UINT y = 0; y < outputBmp.Height(); y++)
    {
        for(UINT x = 0; x < outputBmp.Width(); x++)
        {
            const Superpixel &p =  _superpixels[_assignments(y, x)];
            outputBmp[y][x] = RGBColor(p.color);
        }
    }
}
