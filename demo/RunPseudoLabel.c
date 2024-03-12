/*****************************************************************************\
* RunMeanLabel.c
*
* AUTHOR  : Felipe Belem
* DATE    : 2021-03-08
* LICENSE : MIT License
* EMAIL   : felipe.belem@ic.unicamp.br
\*****************************************************************************/
#include "ift.h"
#include "iftArgs.h"

void usage();
iftImage *calcPseudoLabel
(const iftImage *label_img);
void readImgInputs
(const iftArgs *args, iftImage **labels, const char **path);
void writePseudoImage
(const iftImage *pseudo_img, const char *path);

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
  iftImage *label_img, *out_img;

  readImgInputs(args, &label_img, &OUT_PATH);
  iftDestroyArgs(&args);

  out_img = calcPseudoLabel(label_img);
  iftDestroyImage(&label_img);

  writePseudoImage(out_img, OUT_PATH);
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
         "Output pseudo colored label image (2D or video folder)");

  printf("\nThe optional parameters are:\n");
  printf("%-*s %s\n", SKIP_IND, "--help", 
         "Prints this message");

  printf("\n");
}

iftImage *calcPseudoLabel
(const iftImage *label_img)
{
  #if IFT_DEBUG //-----------------------------------------------------------//
  assert(label_img != NULL);
  #endif //------------------------------------------------------------------//
  const int NORM_VAL = 255, ANGLE_SKIP = 64, SAT_ROUNDS = 5, VAL_ROUNDS = 3;
  int min_label, max_label, num_labels, hue, round_sat, round_val;
  float sat, val;
  int *label_hsv;
  float *label_saturation, *label_val;
  iftImage *pseudo_img;

  pseudo_img = iftCreateColorImage(label_img->xsize, label_img->ysize, 
                                   label_img->zsize, 8);

  iftMinMaxValues(label_img, &min_label, &max_label);
  num_labels = max_label - min_label + 1;
  
  label_hsv = calloc(num_labels, sizeof(int));
  assert(label_hsv != NULL);
  label_saturation = calloc(num_labels, sizeof(float));
  assert(label_saturation != NULL);
  label_val = calloc(num_labels, sizeof(float));
  assert(label_val != NULL);

  hue = 0;
  round_sat = round_val = 0;
  sat = val = 0.0;
  for(int i = 0; i < num_labels; ++i)
  {
    
    if(round_sat + 1 == SAT_ROUNDS) round_val = (round_val + 1) % VAL_ROUNDS;
    if(hue + ANGLE_SKIP >= 360) round_sat = (round_sat + 1) % SAT_ROUNDS;

    hue = (hue + ANGLE_SKIP) % 360;
    sat = (SAT_ROUNDS - round_sat)/(float)SAT_ROUNDS;
    val = (VAL_ROUNDS - round_val)/(float)VAL_ROUNDS;

    label_hsv[i] = hue;
    label_saturation[i] = sat;
    label_val[i] = val;
  }

  #if IFT_OMP //-------------------------------------------------------------//
  #pragma omp parallel for
  #endif //------------------------------------------------------------------//
  for(int p = 0; p < pseudo_img->n; ++p)
  {
    int p_label;
    iftColor HSV, RGB, YCbCr;

    p_label = label_img->val[p];

    HSV.val[0] = label_hsv[p_label - min_label];
    HSV.val[1] = label_saturation[p_label - min_label] * NORM_VAL;
    HSV.val[2] = label_val[p_label - min_label] * NORM_VAL;

    RGB = iftHSVtoRGB(HSV, NORM_VAL);
    YCbCr = iftRGBtoYCbCr(RGB, NORM_VAL);

    pseudo_img->val[p] = YCbCr.val[0];
    pseudo_img->Cb[p] = YCbCr.val[1];
    pseudo_img->Cr[p] = YCbCr.val[2];
  }

  free(label_hsv);
  free(label_saturation);

  return pseudo_img;
}

void readImgInputs
(const iftArgs *args, iftImage **labels, const char **path)
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

void writePseudoImage
(const iftImage *pseudo_img, const char *path)
{
  const char *EXT = iftFileExt(path);

  if(iftIs3DImage(pseudo_img) == false)
    iftWriteImageByExt(pseudo_img, path);
  else if(iftCompareStrings(EXT, ".scn") == false)  
    iftWriteVolumeAsSingleVideoFolder(pseudo_img, path);
  else
    iftError("Unsupported file format", "writeOvlayImage");
}