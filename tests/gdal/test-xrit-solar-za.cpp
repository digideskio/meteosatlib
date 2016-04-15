#include "test-utils.h"
#include <stdint.h>

#define PERFORM_SLOW_TESTS

using namespace std;

namespace tut {

#define TESTFILE01  "H:MSG2:VIS006:200807150900"
#define TESTFILE01r "H:MSG2:VIS006a:200807150900"
#define TESTFILE04i "H:MSG1:IR_039a:200611130800"
#define TESTFILE04  "H:MSG2:IR_039:201001191200"
#define TESTFILE04r "H:MSG2:IR_039a:201001191200"
#define TESTFILE12  "H:MSG1:HRVa:200611141200"

struct xrit_solar_za_shar
{
    xrit_solar_za_shar()
    {
    }

    ~xrit_solar_za_shar()
    {
    }
};
TESTGRP(xrit_solar_za);

// Test opening channel 1 (VIS 0.6, with reflectance)
template<> template<>
void to::test<1>()
{
    unique_ptr<GDALDataset> dataset = openro(TESTFILE01);
    gen_ensure(dataset.get() != 0);
    gen_ensure_equals(string(GDALGetDriverShortName(dataset->GetDriver())), "MsatXRIT");

    // Check that we have the real and the virtual raster bands
    gen_ensure_equals(dataset->GetRasterCount(), 1);

    // x:2000,y:3400
    GDALRasterBand* rb = dataset->GetRasterBand(1);
    uint16_t val;
    rb->RasterIO(GF_Read, 2000, 3400, 1, 1, &val, 1, 1, GDT_UInt16, 0, 0);
    gen_ensure_equals(val, 96);

    unique_ptr<GDALDataset> datasetr = openro(TESTFILE01r);
    gen_ensure(datasetr.get() != 0);
    gen_ensure_equals(string(GDALGetDriverShortName(datasetr->GetDriver())), "MsatXRIT");

    // Check that we have the real and the virtual raster bands
    gen_ensure_equals(datasetr->GetRasterCount(), 1);

    rb = datasetr->GetRasterBand(1);
    float valr;
    rb->RasterIO(GF_Read, 2000, 3400, 1, 1, &valr, 1, 1, GDT_Float32, 0, 0);
    gen_ensure_similar(valr, 0.156248, 0.0001);
}

// Test opening channel 4 (IR 0.39, with missing accessory channels)
template<> template<>
void to::test<2>()
{
    try {
        unique_ptr<GDALDataset> dataset = openro(TESTFILE04i);
        gen_ensure(false);
    } catch (...) {
    }
}

// Test opening channel 4 (IR 0.39, with all accessory channels yet)
template<> template<>
void to::test<3>()
{
    unique_ptr<GDALDataset> dataset = openro(TESTFILE04);
    gen_ensure(dataset.get() != 0);
    gen_ensure_equals(string(GDALGetDriverShortName(dataset->GetDriver())), "MsatXRIT");

    // Check that we have the real and the virtual raster bands
    gen_ensure_equals(dataset->GetRasterCount(), 1);

    // x:2000,y:350
    GDALRasterBand* rb = dataset->GetRasterBand(1);
    uint16_t val;
    rb->RasterIO(GF_Read, 2000, 350, 1, 1, &val, 1, 1, GDT_UInt16, 0, 0);
    gen_ensure_equals(val, 287);

    unique_ptr<GDALDataset> datasetr = openro(TESTFILE04r);
    gen_ensure(datasetr.get() != 0);
    gen_ensure_equals(string(GDALGetDriverShortName(datasetr->GetDriver())), "MsatXRIT");

    // Check that we have the real and the virtual raster bands
    gen_ensure_equals(datasetr->GetRasterCount(), 1);

    rb = datasetr->GetRasterBand(1);
    float valr;
    rb->RasterIO(GF_Read, 2000, 350, 1, 1, &valr, 1, 1, GDT_Float32, 0, 0);
    gen_ensure_similar(valr, 0.33964, 0.0001);
}

// Test opening channel 12 (HRV, with reflectance)
template<> template<>
void to::test<4>()
{
    unique_ptr<GDALDataset> dataset = openro(TESTFILE12);
    gen_ensure(dataset.get() != 0);
    gen_ensure_equals(string(GDALGetDriverShortName(dataset->GetDriver())), "MsatXRIT");

    // Check that we have the real and the virtual raster bands
    gen_ensure_equals(dataset->GetRasterCount(), 1);
}

}

/* vim:set ts=4 sw=4: */
