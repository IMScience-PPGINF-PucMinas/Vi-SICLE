/*****************************************************************************\
* RunCorMedia.c
*
* AUTHOR  : Lucca Lacerda
* DATE    : 2022-04-05
* LICENSE : MIT License
* EMAIL   : lsplacerda@sga.pucminas.br
\*****************************************************************************/
#include "ift.h"
#include "iftArgs.h"

void usage();
iftImage *calcCorMedia
(const iftImage *label_img, iftImage *imgOrg);
void readImgInputs
(const iftArgs *args, iftImage **labels, const char **path, iftImage *imgOrg);
void writeCorMediaImage
(const iftImage *cor_media, const char *path);

int main(int argc, char const *argv[])
{
  //-------------------------------------------------------------------------//
  bool has_req, has_help;
  iftArgs *args;
  
  args = iftCreateArgs(argc, argv);

  has_req = iftExistArg(args, "labels") && iftExistArg(args, "out");
  has_help = iftExistArg(args, "help");

  if(has_req == false || has_help == true)
  {
    usage(); 
    iftDestroyArgs(&args);
    return EXIT_FAILURE;
  }
  //-------------------------------------------------------------------------//
  const char *OUT_PATH;
  iftImage *label_img, *out_img,*imgOrg;

  readImgInputs(args, &label_img, &OUT_PATH, &imgOrg);
  iftDestroyArgs(&args);

  out_img = calcCorMedia(label_img, imgOrg);
  iftDestroyImage(&label_img);

  writeCorMediaImage(out_img, OUT_PATH);
  iftDestroyImage(&out_img);

  return EXIT_SUCCESS;
}

void usage()
{
  const int SKIP_IND = 15; // For indentation purposes
  printf("\nThe required parameters are:\n");
  printf("%-*s %s\n", SKIP_IND, "--labels", 
         "Input label image (2D, 3D or video folder)");
  printf("%-*s %s\n", SKIP_IND, "--out", 
         "Output CorMedia colored label image (2D or video folder)");

  printf("\nThe optional parameters are:\n");
  printf("%-*s %s\n", SKIP_IND, "--help", 
         "Prints this message");

  printf("\n");
}

iftImage *calcCorMedia
(const iftImage *label_img, iftImage *imgOrg)
{
  #if IFT_DEBUG //-----------------------------------------------------------//
  assert(label_img != NULL);
  #endif //------------------------------------------------------------------//
  const int NORM_VAL = 255, ANGLE_SKIP = 64, SAT_ROUNDS = 5, VAL_ROUNDS = 3;
  int min_label, max_label, num_labels, hue, round_sat, round_val;
  float sat, val;
  float **media_super_voxel, *size_super_voxel;

  iftMinMaxValues(label_img, &min_label, &max_label);
  num_labels = max_label - min_label + 1;
  int ncolors = (iftIsColorImage(imgOrg) == false ) ?  1 : 3;

  media_super_voxel = calloc(num_labels, sizeof(float));
  size_super_voxel = calloc(num_labels, sizeof(float));
  
  for(int p =0; p < num_labels;++p){
    media_super_voxel[p] = calloc(num_labels, sizeof(float));
  }
  // int *label_hsv;
  // float *label_saturation, *label_val;
  iftImage *cor_media;
  for(int p = 0; p < label_img->n; ++p){
    // Calc media cada super voxel
    media_super_voxel += label_img->val[p];
    size_super_voxel[label_img->val[p]]++;
  }
  for(int p =0; p < num_labels;++p){
    // media_super_voxel[p] /= size_super_voxel[p];
  }
  cor_media = iftCreateColorImage(label_img->xsize, label_img->ysize, 
                                        label_img->zsize, 8);
  for(int p = 0; p < label_img->n; ++p){
    //voxel Pertence ao super Voxel
    for(int q = 0; q < label_img->n; ++q){ 
      // if(label_img->val[p] == label_img->val[q])
        
    }
  }


  return cor_media;
}

void readImgInputs
(const iftArgs *args, iftImage **labels, const char **path, iftImage *imgOrg)
{
  #if IFT_DEBUG //-----------------------------------------------------------//
  assert(args != NULL);
  assert(labels != NULL);
  assert(path != NULL);
  #endif //------------------------------------------------------------------//
  const char *PATH;

  if(iftHasArgVal(args, "labels") == true) 
  {
    PATH = iftGetArg(args, "labels");

    if(iftDirExists(PATH) == false) (*labels) = iftReadImageByExt(PATH);
    else (*labels) = iftReadImageFolderAsVolume(PATH);
  }
  else iftError("No label image path was given", "readImgInputs");

  if(iftHasArgVal(args, "out") == true)
  {
    PATH = iftGetArg(args, "out");

    if(iftIsImagePathnameValid(PATH) == true) (*path) = PATH;
    else iftError("Unknown image type", "readImgInputs");
  }
  else iftError("No output image path was given", "readImgInputs");
}

void writeCorMediaImage
(const iftImage *cor_media, const char *path)
{
  const char *EXT = iftFileExt(path);

  if(iftIs3DImage(cor_media) == false)
    iftWriteImageByExt(cor_media, path);
  else if(iftCompareStrings(EXT, ".scn") == false)  
    iftWriteVolumeAsSingleVideoFolder(cor_media, path);
  else
    iftError("Unsupported file format", "writeOvlayImage");
}