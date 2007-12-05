#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <png.h>

typedef struct {
    unsigned char ch;
    unsigned char colour;
} trefchar;

SDL_Color colour_lookup[16] = {
/*    {0x00, 0x00, 0x80, 0xFF}, */  /* blue instead of black, like tty */
    {0x00, 0x00, 0x00, 0xFF},
    {0x80, 0x00, 0x00, 0xFF},
    {0x00, 0x80, 0x00, 0xFF},
    {0xA0, 0x80, 0x40, 0xFF},
    {0x00, 0x00, 0x80, 0xFF},
    {0x80, 0x00, 0x80, 0xFF},
    {0x00, 0x80, 0x80, 0xFF},
    {0xC0, 0xC0, 0xC0, 0xFF},
    {0x80, 0x80, 0x80, 0xFF},
    {0xFF, 0xA0, 0x00, 0xFF},
    {0x00, 0xFF, 0x00, 0xFF},
    {0xFF, 0xFF, 0x00, 0xFF},
    {0x00, 0x00, 0xFF, 0xFF},
    {0xFF, 0x00, 0xFF, 0xFF},
    {0x00, 0xFF, 0xFF, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF}
};

int parse_parameters(int argc,char *argv[]);
int reference_load(char *filename);
void render_tileset();
int main_loop();
void save_png(char *filename, SDL_Surface *surface);
int file_exists(char *filename);

#define FILENAME_DEF "nhtexttile.def"
#define FILENAME_IMG "nhtexttile.png"
#define FILENAME_TTF "nhtexttile.ttf"

char *filename_def=NULL;
char *filename_img=NULL;
char *filename_ttf=NULL;
char *filename_bgd=NULL;

int width=16;
int height=16;
int columns=40;
int pointsize=0;

int is_alpha=0;

int reference_size;
trefchar reference[4096];

SDL_Surface *screen;
SDL_Surface *image;

TTF_Font *font=NULL;

int
main(argc,argv)
    int argc;
    char *argv[];
{
    SDL_Surface *image_noalpha;
    SDL_Surface *background;

    SDL_Rect srcrect,dstrect;

    if (!parse_parameters(argc,argv)) exit(-1);

    if (SDL_Init(SDL_INIT_VIDEO)) {
        printf("unable to initialise SDL: %s\n", SDL_GetError());
        return -1;
    }
    atexit(SDL_Quit);

    if (TTF_Init()) {
        printf("unable to initialise TTF: %s\n", SDL_GetError());
        return -1;
    }

    if (!reference_load(filename_def)) {
        printf("error loading reference file \"%s\"\n",filename_def);
        return -1;
    }

    if (!file_exists(filename_ttf)) {
        printf("error loading font file \"%s\"\n",filename_ttf);
        return -1;
    }
    font=TTF_OpenFont(filename_ttf,pointsize);
    if (!font) {
        printf("error loading font file \"%s\"\n",filename_ttf);
        return -1;
    }

    printf("creating image at %dx%d\n",columns*width,((reference_size+(columns-1))/columns)*height);

    screen = SDL_SetVideoMode(columns*width,((reference_size+(columns-1))/columns)*height,32,0);
    if (screen == NULL) {
        printf("unable to set video mode: %s\n", SDL_GetError());
        return -1;
    }
    SDL_WM_SetCaption("nhtexttile - http://www.linuxgames.co.za/",0);
    SDL_SetAlpha(screen,SDL_SRCALPHA,SDL_ALPHA_TRANSPARENT);

    image_noalpha = SDL_CreateRGBSurface(
        SDL_HWSURFACE,
        columns*width,((reference_size+(columns-1))/columns)*height,32,
        screen->format->Rmask,
        screen->format->Gmask,
        screen->format->Bmask,
        0xFFFFFFFF-(screen->format->Rmask|screen->format->Gmask|screen->format->Bmask));
    if (image_noalpha == NULL) {
        printf("unable to create image: %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetAlpha(image_noalpha,SDL_SRCALPHA,SDL_ALPHA_OPAQUE);
    image=SDL_DisplayFormatAlpha(image_noalpha);
    SDL_SetAlpha(image,SDL_SRCALPHA,SDL_ALPHA_TRANSPARENT);

    if (filename_bgd) {
        printf("using background: \"%s\"\n",filename_bgd);
        background=IMG_Load(filename_bgd);
        SDL_SetAlpha(background,0,SDL_ALPHA_TRANSPARENT);
        srcrect.x=srcrect.y=dstrect.x=dstrect.y=0;
        srcrect.w=dstrect.w=background->w;
        srcrect.h=dstrect.h=background->h;
        for (dstrect.y=0;dstrect.y<image->h;dstrect.y+=background->h) {
            for (dstrect.x=0;dstrect.x<image->w;dstrect.x+=background->w) {
                SDL_BlitSurface(background,&srcrect,image,&dstrect);
            }
        }
    } else if (!is_alpha) {
        dstrect.x=dstrect.y=0;
        dstrect.w=screen->w;
        dstrect.h=screen->h;
        SDL_FillRect(image,&dstrect,0xFF404040);
    }

    render_tileset();

    dstrect.x=dstrect.y=0;
    dstrect.w=screen->w;
    dstrect.h=screen->h;
    SDL_FillRect(screen,&dstrect,0xFF404040);

    srcrect.x=srcrect.y=dstrect.x=dstrect.y=0;
    srcrect.w=screen->w;
    srcrect.h=screen->h;
    dstrect.w=screen->w;
    dstrect.h=screen->h;
    SDL_BlitSurface(image,&srcrect,screen,&dstrect);

    SDL_Flip(screen);

    if (main_loop()) {
        printf("saving \"%s\"...\n",filename_img);
        save_png(filename_img,image);
    }

    return 0;
}

void
help()
{
    printf("format:\n\tnhtexttile [-a] [-w width] [-h height] [-c columns] [-p pointsize] [-i input.def] [-o outputimage.png] [-f font.ttf] [-b background ]\n\n");
    exit(0);
}

int
parse_parameters(argc,argv)
    int argc;
    char *argv[];
{
    int c;

    for (c=1;c<argc;c++) {
        switch (argv[c][0]) {
            case '-':
                switch (argv[c][1]) {
                    case 'i':
                        if (c+1>=argc) {
                            printf("*error: no reference filename supplied\n");
                            return 0;
                        }
                        c++;
                        if (filename_def) {
                            printf("two reference files given: \"%s\" and \"%s\"\n",filename_def,argv[c]);
                            return 0;
                        } else {
                            filename_def=malloc(strlen(argv[c])+1);
                            strcpy(filename_def,argv[c]);
                        }
                        break;
                    case 'o':
                        if (c+1>=argc) {
                            printf("*error: no image filename supplied\n");
                            return 0;
                        }
                        c++;
                        if (filename_img) {
                            printf("two image files given: \"%s\" and \"%s\"\n",filename_img,argv[c]);
                            return 0;
                        } else {
                            filename_img=malloc(strlen(argv[c])+1);
                            strcpy(filename_img,argv[c]);
                        }
                        break;
                    case 'b':
                        if (c+1>=argc) {
                            printf("*error: no background image filename supplied\n");
                            return 0;
                        }
                        c++;
                        if (filename_bgd) {
                            printf("two background image files given: \"%s\" and \"%s\"\n",filename_bgd,argv[c]);
                            return 0;
                        } else {
                            filename_bgd=malloc(strlen(argv[c])+1);
                            strcpy(filename_bgd,argv[c]);
                        }
                        break;
                    case 'f':
                        if (c+1>=argc) {
                            printf("*error: no font filename supplied\n");
                            return 0;
                        }
                        c++;
                        if (filename_ttf) {
                            printf("two font files given: \"%s\" and \"%s\"\n",filename_ttf,argv[c]);
                            return 0;
                        } else {
                            filename_ttf=malloc(strlen(argv[c])+1);
                            strcpy(filename_ttf,argv[c]);
                        }
                        break;
                    case 'a':
                        is_alpha=1;
                        break;
                    case 'w':
                        if (c+1>=argc) {
                            printf("*error: no width value supplied\n");
                            return 0;
                        }
                        c++;
                        width=atoi(argv[c]);
                        break;
                    case 'h':
                        if (c+1>=argc) {
                            printf("*error: no height value supplied\n");
                            return 0;
                        }
                        c++;
                        height=atoi(argv[c]);
                        break;
                    case 'c':
                        if (c+1>=argc) {
                            printf("*error: no columns value supplied\n");
                            return 0;
                        }
                        c++;
                        columns=atoi(argv[c]);
                        break;
                    case 'p':
                        if (c+1>=argc) {
                            printf("*error: no pointsize value supplied\n");
                            return 0;
                        }
                        c++;
                        pointsize=atoi(argv[c]);
                        break;
                    default:
                        help();
                }
                break;
            default:
                help();
                break;
        }
    }

    if (!filename_def) {
        filename_def=malloc(sizeof(FILENAME_DEF)+1);
        strcpy(filename_def,FILENAME_DEF);
    }

    if (!filename_img) {
        filename_img=malloc(sizeof(FILENAME_IMG)+1);
        strcpy(filename_img,FILENAME_IMG);
    }

    if (!filename_ttf) {
        filename_ttf=malloc(sizeof(FILENAME_TTF)+1);
        strcpy(filename_ttf,FILENAME_TTF);
    }

    if (!width) width=16;
    if (!height) height=16;
    if (!columns) columns=40;
    if (!pointsize) pointsize=width;

    printf("processing \"%s\" to \"%s\"...\n\ttile dimensions: %dx%d\n\tcolumns: %d\n\tfont: \"%s\"\n",
        filename_def,
        filename_img,
        width,
        height,
        columns,
        filename_ttf);

    return 1;
}

int reference_load(char *filename) {
    FILE *f;

    f=fopen(filename,"r");
    if (!f) return 0;

    for(reference_size=0;fread(&reference[reference_size],1,2,f);reference_size++) {
        switch (reference[reference_size].colour) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                reference[reference_size].colour-='0';
                break;
            default:
                reference[reference_size].colour-=('a'-10);
                break;
        }
    };

    fclose(f);
    return 1;
}

void
render_tile(x,y,ch,colour)
    int x;
    int y;
    char ch;
    int colour;
{
    SDL_Surface *ch_sur;
    SDL_Rect srcrect,dstrect;
    char rend_ch[2];

    rend_ch[0]=ch;
    rend_ch[1]=0;
    ch_sur=TTF_RenderText_Blended(font,(char *)&rend_ch,colour_lookup[colour]);
    if (filename_bgd||(!is_alpha)) {
        SDL_SetAlpha(ch_sur,SDL_SRCALPHA,SDL_ALPHA_OPAQUE);
    } else {
        SDL_SetAlpha(ch_sur,0,SDL_ALPHA_OPAQUE);
    }

    srcrect.x=srcrect.y=0;
    srcrect.w=ch_sur->w; srcrect.h=ch_sur->h;
    dstrect.w=srcrect.w;
    dstrect.h=srcrect.h;

    dstrect.x=width*x+(width-srcrect.w)/2;
    dstrect.y=height*y+(height-srcrect.h)/2;

    SDL_BlitSurface(ch_sur,&srcrect,image,&dstrect);
};

void
render_tileset()
{
    int c;

    for (c=0;c<reference_size;c++) {
        render_tile(c%columns,c/columns,reference[c].ch,reference[c].colour);
    }
}

int
main_loop()
{
    SDL_Event event;

    SDL_PollEvent(&event);
    while ((event.type != SDL_QUIT)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        return 0;
                    case SDLK_s:
                        /* save & quit */
                        return 1;
                    default:
                        break;
                }
        }
    SDL_PollEvent(&event);
    }
    return 0;
}

void
save_png(filename,surface)
    char *filename;
    SDL_Surface *surface;
{
    png_bytep * volatile row_pointers = NULL;
    png_structp     png_ptr  = NULL;
    png_infop       info_ptr = NULL;

    FILE *fp;
    int row,column;
    Uint8 r,g,b,a;
    Uint32 *pixel;


    SDL_LockSurface(surface);

    row_pointers = (png_bytep *) malloc(surface->h * sizeof(png_bytep));
    for (row=0; row < surface->h; row++) {
        row_pointers[row]=malloc(surface->w*4);
        for (column=0;column<surface->w;column++) {
            pixel=surface->pixels+(surface->pitch*(row))+(column*surface->format->BytesPerPixel);
            SDL_GetRGBA(*pixel,surface->format,&r,&g,&b,&a);
            row_pointers[row][column*4+0]=r;
            row_pointers[row][column*4+1]=g;
            row_pointers[row][column*4+2]=b;
            row_pointers[row][column*4+3]=a;
        }
    }

    fp=fopen(filename,"wb");
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);

    if (setjmp(png_ptr->jmpbuf)) {
        fprintf(stderr, "Unknown problem while writing PNG.\n");
        fclose(fp);
    } else {

        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, surface->w, surface->h, 8,
            PNG_COLOR_TYPE_RGB_ALPHA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        png_write_image(png_ptr, row_pointers);
        png_write_end(png_ptr, info_ptr);

        png_destroy_write_struct(&png_ptr, &info_ptr);

        fclose(fp);

    }

    for (row=0; row < surface->h; row++) {
        free(row_pointers[row]);
    }
    free(row_pointers);

    SDL_UnlockSurface(surface);

}

int file_exists(char *filename) {
    FILE *f;

    f=fopen(filename,"r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

