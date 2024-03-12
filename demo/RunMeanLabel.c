/*****************************************************************************\
* RunMeanLabel.c
*
* AUTHOR  : Felipe Belem
* DATE    : 2021-02-28
* LICENSE : MIT License
* EMAIL   : felipe.belem@ic.unicamp.br
\*****************************************************************************/
#include "ift.h"
#include "iftArgs.h"

void usage();
iftImage *calcMeanLabel
(const iftImage *orig_img, const iftImage *label_img);
void readImgInputs
(const iftArgs *args, iftImage **img, iftImage **labels, const char **path);
void writeMeanImage
(const iftImage *mean_img, const char *path);

int main(int argc, char const *argv[])
{
  //-------------------------------------------------------------------------//
  bool has_req, has_help;
  iftArgs *args;

  args = iftCreateArgs(argc, argv);

  has_req = iftExistArg(args, "img")  && iftExistArg(args, "labels") &&
            iftExistArg(args, "out");
  has_help = iftExistArg(args, "help");

  if(has_req == false || has_help == true)
  {
    usage(); 
    iftDestroyArgs(&args);
    return EXIT_FAILURE;
  }
  //-------------------------------------------------------------------------//
  const char *OUT_PATH;
  iftImage *img, *label_img, *out_img;

  readImgInputs(args, &img, &label_img, &OUT_PATH);
  iftDestroyArgs(&args);

  out_img = calcMeanLabel(img, label_img);
  iftDestroyImage(&img);
  iftDestroyImage(&label_img);

  writeMeanImage(out_img, OUT_PATH);
  iftDestroyImage(&out_img);

  return EXIT_SUCCESS;
}

void usage()
{
  const int SKIP_IND = 15; // For indentation purposes
  printf("\nThe required parameters are:\n");
  printf("%-*s %s\n", SKIP_IND, "--img", 
         "Input image (2D, 3D, or video folder)");
  printf("%-*s %s\n", SKIP_IND, "--labels", 
         "Input label image (2D, 3D or video folder)");
  printf("%-*s %s\n", SKIP_IND, "--out", 
         "Output mean label colored image (2D or video folder)");

  printf("\nThe optional parameters are:\n");
  printf("%-*s %s\n", SKIP_IND, "--help", 
         "Prints this message");

  printf("\n");
}

iftImage *calcMeanLabel
(const iftImage *orig_img, const iftImage *label_img)
{
  #if IFT_DEBUG //-----------------------------------------------------------//
  assert(orig_img != NULL);
  assert(label_img != NULL);
  #endif //------------------------------------------------------------------//
  int min_label, max_label, num_labels, num_feats;
  int *label_size;
  float **label_feats;
  iftImage *mean_img;

  mean_img = iftCreateImageFromImage(orig_img);

  iftMinMaxValues(label_img, &min_label, &max_label);
  num_labels = max_label - min_label + 1;
  
  label_size = calloc(num_labels, sizeof(int));
  assert(label_size != NULL);
  label_feats = calloc(num_labels, sizeof(float*));
  assert(label_feats != NULL);

  if(iftIsColorImage(orig_img) == true) num_feats = 3;
  else num_feats = 1;

  for(int i = 0; i < num_labels; ++i)
  {
    label_size[i] = 0;
    label_feats[i] = calloc(num_feats, sizeof(float));
    assert(label_feats[i]);
  }

  for(int p = 0; p < mean_img->n; ++p)
  {
    int p_label;

    p_label = label_img->val[p];

    label_size[p_label - min_label]++;
    label_feats[p_label - min_label][0] += orig_img->val[p];
    if(num_feats > 1)
    {
      label_feats[p_label - min_label][1] += orig_img->Cb[p];
      label_feats[p_label - min_label][2] += orig_img->Cr[p];
    }
  }

  for(int i = 0; i < num_labels; ++i)
    for(int j = 0; j < num_feats; ++j)
      label_feats[i][j] /= (float)label_size[i];
  free(label_size);

  for(int p = 0; p < mean_img->n; ++p)
  {
    int p_label;

    p_label = label_img->val[p];

    mean_img->val[p] = label_feats[p_label - min_label][0];
    if(num_feats > 1)
    {
      mean_img->Cb[p] = label_feats[p_label - min_label][1];
      mean_img->Cr[p] = label_feats[p_label - min_label][2];
    }
  }

  for(int i = 0; i < num_labels; ++i)
    free(label_feats[i]);
  free(label_feats);

  if(iftImageDepth(orig_img) != 8)  iftConvertNewBitDepth(&mean_img, 8);

  return mean_img;
}

void readImgInputs
(const iftArgs *args, iftImage **img, iftImage **labels, const char **path)
{
  #if IFT_DEBUG //-----------------------------------------------------------//
  assert(args != NULL);
  assert(img != NULL);
  assert(labels != NULL);
  assert(path != NULL);
  #endif //------------------------------------------------------------------//
  const char *PATH;

  if(iftHasArgVal(args, "img") == true) 
  {
    PATH = iftGetArg(args, "img");

    if(iftDirExists(PATH) == false) (*img) = iftReadImageByExt(PATH);
    else (*img) = iftReadImageFolderAsVolume(PATH);
  }
  else iftError("No image path was given", "readImgInputs");

  if(iftHasArgVal(args, "labels") == true) 
  {
    PATH = iftGetArg(args, "labels");

    if(iftDirExists(PATH) == false) (*labels) = iftReadImageByExt(PATH);
    else (*labels) = iftReadImageFolderAsVolume(PATH);
  }
  else iftError("No label image path was given", "readImgInputs");

  iftVerifyImageDomains(*img, *labels, "readImgInputs");

  if(iftHasArgVal(args, "out") == true)
  {
    PATH = iftGetArg(args, "out");

    if(iftIsImagePathnameValid(PATH) == true) (*path) = PATH;
    else iftError("Unknown image type", "readImgInputs");
  }
  else iftError("No output image path was given", "readImgInputs");
}

void writeMeanImage
(const iftImage *mean_img, const char *path)
{
  const char *EXT = iftFileExt(path);

  if(iftIs3DImage(mean_img) == false)
    iftWriteImageByExt(mean_img, path);
  else if(iftCompareStrings(EXT, ".scn") == false)  
    iftWriteVolumeAsSingleVideoFolder(mean_img, path);
  else if(iftIsColorImage(mean_img) == false)
    iftWriteImageByExt(mean_img, path);
  else
    iftError("Unsupported file format", "writeOvlayImage");
}