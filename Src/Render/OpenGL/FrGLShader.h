/*=============================================================================
    FrGLShader.h: OpenGL shader.
    Copyright Oct.2016 Vlad Gordienko.
=============================================================================*/

//
// Shaders directories.
//
#define VERT_SHADER_DIR		L"\\Render\\OpenGL\\Vertex.glsl"
#define FRAG_SHADER_DIR		L"\\Render\\OpenGL\\Fragment.glsl"


/*-----------------------------------------------------------------------------
    GLSL cached uniforms.
-----------------------------------------------------------------------------*/

//
// Uniform light source.
//
struct TUniformLightSource
{
public:
	GLuint		Effect;
	GLuint		Color;
	GLuint		Brightness;
	GLuint		Radius;
	GLuint		Location;
	GLuint		Rotation;
};


//
// Uniform post effect.
//
struct TUniformPostEffect
{
public:
	GLuint		Highlights;
	GLuint		MidTones;
	GLuint		Shadows;
	GLuint		BWScale;
};


/*-----------------------------------------------------------------------------
    COpenGLShader.
-----------------------------------------------------------------------------*/

//
// A complex OpenGL shader.
//
class COpenGLShader
{
public:
	// Maximum light sources per lightmap.
	enum { MAX_LIGHTS	= 16 };

	// Variables.
	Integer			ANum, MNum;
	TPostEffect		PostEffect;

	// COpenGLShader interface.
	COpenGLShader();
	~COpenGLShader();
	void ResetLights();
	Bool AddLight( FLightComponent* Light, TVector Location, TAngle Rotation );
	void SetPostEffect( const TPostEffect& InEffect );
	void SetAmbientLight( TColor InAmbient );

	// Shader modes.
	void SetModeNone();
	void SetModeUnlit();
	void SetModeLightmap();
	void SetModeComplex();

private:
	// Internal variables.
	Bool				bInUse;
	Bool				bUnlit;
	Bool				bLightmap;

	// Uniform variables.
	GLuint				idUnlit;
	GLuint				idRenderLightmap;
	GLuint				idBitmap;
	GLuint				idSaturation;
	TUniformPostEffect	idEffect;
	TUniformLightSource	idALights[MAX_LIGHTS];
	TUniformLightSource	idMLights[MAX_LIGHTS];
	GLuint				idANum;
	GLuint				idMNum;
	GLuint				idGameTime;
	GLuint				idAmbientLight;

	// Shader variables.
	GLuint				iglProgram;
	GLuint				iglVertShader;
	GLuint				iglFragShader;

	// Friends.
	friend	COpenGLCanvas;
	friend	COpenGLRender;
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/