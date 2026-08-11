
#define TextLettersPerLine 14
#define TextNumbersLines 7

#define NewlineDist 1.35f

typedef struct {
    simd_float3 color;
    float fontSize;
    uint16_t startsAt;
} truncatedTextPerameters;

typedef struct
{
    vector_float2 position; // -x, -y
    uint8_t uniformVertexIndex;
    uint8_t uniformTextureIndex;
} GUIVertex; // 10

typedef struct {
    int heightPerLetter;
    int widthOfPadding;
} characterAtlasParams;

typedef struct
{
    vector_float2 sizes;
} GUIVertexUniforms; // 24

typedef struct
{
    vector_float2 texCoords;
    vector_float2 texSizes;
} GUITextureUniforms; // 24

typedef struct {
    float coords[6];
} bezierCurve;
