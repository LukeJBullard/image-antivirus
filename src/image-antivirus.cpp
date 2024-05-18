#include <spng.h>
#include <stdlib.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include <list>
#include <algorithm>

//108mp
#define MAX_IMAGE_WIDTH 12000
#define MAX_IMAGE_HEIGHT 9000
#define MAX_CHUNK_SIZE 35000000 //35MB, should be suitable for 108MP

//max cache size 350MB + 35MB, as the inflated output is in the cache
#define MAX_CACHE_SIZE 385000000
#define MAX_OUTPUT_SIZE 350000000 //350MB 108MP

using namespace std;
using namespace std::filesystem;

int main(int argc, char* argv[])
{
    spng_ctx *ctx;
    spng_ctx *ctx_out;
    size_t output_size = 0;
    void *output_buffer = NULL;

    spng_ihdr *ihdr = (spng_ihdr *)malloc(sizeof(spng_ihdr));
    spng_plte *plte = (spng_plte *)malloc(sizeof(spng_plte));
    spng_trns *trns = (spng_trns *)malloc(sizeof(spng_trns));
    spng_chrm_int *chrm_int = (spng_chrm_int *)malloc(sizeof(spng_chrm_int));
    uint32_t *gama_int = (uint32_t *)malloc(sizeof(uint32_t));
    spng_sbit *sbit = (spng_sbit *)malloc(sizeof(spng_sbit));
    uint8_t *srgb = (uint8_t *)malloc(sizeof(uint8_t));
    spng_bkgd *bkgd = (spng_bkgd *)malloc(sizeof(spng_bkgd));
    spng_hist *hist = (spng_hist *)malloc(sizeof(spng_hist));
    spng_phys *phys = (spng_phys *)malloc(sizeof(spng_phys));
    uint32_t *n_splt = (uint32_t *)malloc(sizeof(uint32_t));
    spng_splt *splt = (spng_splt *)malloc(sizeof(spng_splt));
    spng_time *time = (spng_time *)malloc(sizeof(spng_time));
    spng_offs *offs = (spng_offs *)malloc(sizeof(spng_offs));


    FILE *fp;

    int output_format = SPNG_FMT_RGBA16;
    int output_flags = SPNG_DECODE_TRNS & SPNG_DECODE_GAMMA;

    int encode_flags = SPNG_ENCODE_FINALIZE;

    if (!is_directory("output") || !exists("output"))
    {
        create_directory("output");
    } else if (exists("output"))
    {
        cerr << "Error: 'output' already exists and is not a directory" << endl;
        return 1;
    }

    list<path> files = list<path>();

    error_code ec;

    for (const auto & entry : directory_iterator("."))
    {
        if (is_regular_file(entry, ec))
        {
            if (entry.path().extension() == ".png")
            {
                files.push_back(entry.path());
            }
        }
        if (ec)
        {
            cerr << "Error: is_regular_file failed: " << ec.message() << endl;
            return 1;
        }
    }

    for (list<path>::iterator it = files.begin(); it != files.end(); ++it)
    {
        output_size = 0;

        memset(ihdr, 0, sizeof(spng_ihdr));
        memset(plte, 0, sizeof(spng_plte));
        memset(trns, 0, sizeof(spng_trns));
        memset(chrm_int, 0, sizeof(spng_chrm_int));
        *gama_int = 0;
        memset(sbit, 0, sizeof(spng_sbit));
        *srgb = 0;
        memset(bkgd, 0, sizeof(spng_bkgd));
        memset(hist, 0, sizeof(spng_hist));
        memset(phys, 0, sizeof(spng_phys));
        *n_splt = 0;
        splt = NULL;
        memset(time, 0, sizeof(spng_time));
        memset(offs, 0, sizeof(spng_offs));

        ctx = spng_ctx_new(0);

        if (ctx == NULL)
        {
            cerr << "Error: Cannot create spng context" << endl;
            return 1;
        }

        if (spng_set_image_limits(ctx, MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT) ||
            spng_set_chunk_limits(ctx, MAX_CHUNK_SIZE, MAX_CACHE_SIZE))
        {
            cerr << "Error: Can't set spng limits" << endl;
            return 1;
        }

        fp = fopen((*it).c_str(), "rb");
        if (fp == NULL)
        {
            cerr << "Error: Can't open image '" << (*it).string() << "'" << endl;
            return 1;
        }

        if (spng_set_png_file(ctx, fp))
        {
            cerr << "Error: Can't link png file to spng context" << endl;
            return 1;
        }

        spng_decoded_image_size(ctx, output_format, &output_size);

        if (output_size <= 1)
        {
            cerr << "Error determining decoded image size using spng_decoded_image_size" << endl;
            return 1;
        } else if (output_size > MAX_OUTPUT_SIZE)
        {
            cerr << "Error: Output image size larger than maximum constant" << endl;
            return 1;
        }

        if (output_buffer == NULL)
        {
            output_buffer = calloc(1, output_size);
        } else {
            void *temp = realloc(output_buffer, output_size);
            if (!temp)
            {
                cerr << "Error: Realloc failed sizing new output buffer" << endl;
                return 1;
            }
            output_buffer = temp;
            memset(output_buffer, 0, output_size);
        }

        if (output_buffer == NULL)
        {
            cerr << "Error: Unable to allocate output buffer" << endl;
            return 1;
        }

        if (spng_decode_image(ctx, output_buffer, output_size, output_format, output_flags))
        {
            cerr << "Error: Decoding image has failed" << endl;
            return 1;
        }

        spng_get_ihdr(ctx, ihdr);
        spng_get_plte(ctx, plte);
        spng_get_trns(ctx, trns);
        spng_get_chrm_int(ctx, chrm_int);
        spng_get_gama_int(ctx, gama_int);
        spng_get_sbit(ctx, sbit);
        spng_get_srgb(ctx, srgb);
        spng_get_bkgd(ctx, bkgd);
        spng_get_hist(ctx, hist);
        spng_get_phys(ctx, phys);
        spng_get_time(ctx, time);
        spng_get_offs(ctx, offs);

        *n_splt = 0;
        spng_get_splt(ctx, NULL, n_splt);
        if (*n_splt > 0)
        {
            splt = (spng_splt *)malloc((*n_splt) * sizeof(spng_splt));
            spng_get_splt(ctx, splt, n_splt);
        }

        fclose(fp);

        //leave the input ctx open because spng_ctx_free frees some of the needed chunks

        //rewrite the image
        ctx_out = spng_ctx_new(SPNG_CTX_ENCODER);
        if (ctx_out == NULL)
        {
            cerr << "Error: Cannot create output spng context" << endl;
            return 1;
        }

        spng_set_ihdr(ctx_out, ihdr);
        spng_set_plte(ctx_out, plte);
        spng_set_trns(ctx_out, trns);
        spng_set_chrm_int(ctx_out, chrm_int);
        spng_set_gama_int(ctx_out, *gama_int);
        spng_set_sbit(ctx_out, sbit);
        spng_set_srgb(ctx_out, *srgb);
        spng_set_bkgd(ctx_out, bkgd);
        spng_set_hist(ctx_out, hist);
        spng_set_phys(ctx_out, phys);
        spng_set_time(ctx_out, time);
        spng_set_offs(ctx_out, offs);
        spng_set_splt(ctx_out, splt, *n_splt);

        //now free the input context
        spng_ctx_free(ctx);

        path outputPath((*it).parent_path().string() + "/output/" + (*it).filename().string());

        fp = fopen(outputPath.c_str(), "wb");
        if (fp == NULL)
        {
            cerr << "Error: Can't open output file '" << outputPath.string() << "'" << endl;
            return 1;
        }

        if (spng_set_png_file(ctx_out, fp))
        {
            cerr << "Error: Can't link output png file to spng context" << endl;
            return 1;
        }

        if (spng_encode_image(ctx_out, output_buffer, output_size, output_format, encode_flags))
        {
            cerr << "Error: Cannot encode output image" << endl;
            return 1;
        }

        spng_ctx_free(ctx_out);
        fclose(fp);
    }

    return 0;
}