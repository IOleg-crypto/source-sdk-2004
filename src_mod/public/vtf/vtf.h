//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VTF_H
#define VTF_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;
enum ImageFormat;
class Vector;
struct Rect_t;


//-----------------------------------------------------------------------------
// Texture flags
//-----------------------------------------------------------------------------
enum
{
	// flags from the *.txt config file
	TEXTUREFLAGS_POINTSAMPLE	= 0x0001,
	TEXTUREFLAGS_TRILINEAR		= 0x0002,
	TEXTUREFLAGS_CLAMPS			= 0x0004,
	TEXTUREFLAGS_CLAMPT			= 0x0008,
	TEXTUREFLAGS_ANISOTROPIC	= 0x0010,
	TEXTUREFLAGS_HINT_DXT5		= 0x0020,
	TEXTUREFLAGS_NOCOMPRESS		= 0x0040,
	TEXTUREFLAGS_NORMAL			= 0x0080,
	TEXTUREFLAGS_NOMIP			= 0x0100,
	TEXTUREFLAGS_NOLOD			= 0x0200,
	TEXTUREFLAGS_MINMIP			= 0x0400,
	TEXTUREFLAGS_PROCEDURAL		= 0x0800,
	
	// These are automatically generated by vtex from the texture data.
	TEXTUREFLAGS_ONEBITALPHA	= 0x1000,
	TEXTUREFLAGS_EIGHTBITALPHA	= 0x2000,

	// newer flags from the *.txt config file
	TEXTUREFLAGS_ENVMAP			= 0x4000,
	TEXTUREFLAGS_RENDERTARGET	= 0x8000,
	TEXTUREFLAGS_DEPTHRENDERTARGET	= 0x10000,
	TEXTUREFLAGS_NODEBUGOVERRIDE = 0x20000,
	TEXTUREFLAGS_SINGLECOPY		= 0x40000,
	TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA = 0x80000,
	TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL = 0x100000,
	TEXTUREFLAGS_NORMALTODUDV = 0x200000,
	TEXTUREFLAGS_ALPHATESTMIPGENERATION = 0x400000,

	TEXTUREFLAGS_NODEPTHBUFFER = 0x800000,

	TEXTUREFLAGS_NICEFILTERED = 0x1000000,

	TEXTUREFLAGS_LASTFLAG = 0x1000000,
};


//-----------------------------------------------------------------------------
// Cubemap face indices
//-----------------------------------------------------------------------------
enum CubeMapFaceIndex_t
{
	CUBEMAP_FACE_RIGHT = 0,
	CUBEMAP_FACE_LEFT,
	CUBEMAP_FACE_BACK,	// NOTE: This face is in the +y direction?!?!?
	CUBEMAP_FACE_FRONT,	// NOTE: This face is in the -y direction!?!?
	CUBEMAP_FACE_UP,
	CUBEMAP_FACE_DOWN,

	// This is the fallback for low-end
	CUBEMAP_FACE_SPHEREMAP,

	// NOTE: Cubemaps have *7* faces; the 7th is the fallback spheremap
	CUBEMAP_FACE_COUNT
};


//-----------------------------------------------------------------------------
// Enumeration used for spheremap generation
//-----------------------------------------------------------------------------
enum LookDir_t
{
	LOOK_DOWN_X = 0,
	LOOK_DOWN_NEGX,
	LOOK_DOWN_Y,
	LOOK_DOWN_NEGY,
	LOOK_DOWN_Z,
	LOOK_DOWN_NEGZ,
};


//-----------------------------------------------------------------------------
// Use this image format if you want to perform tool operations on the texture
//-----------------------------------------------------------------------------
#define IMAGE_FORMAT_DEFAULT	((ImageFormat)-2)


//-----------------------------------------------------------------------------
// Interface to get at various bits of a VTF texture
//-----------------------------------------------------------------------------
class IVTFTexture
{
public:
	// Initializes the texture and allocates space for the bits
	// In most cases, you shouldn't force the mip count.
	virtual bool Init( int nWidth, int nHeight, ImageFormat fmt, int nFlags, int iFrameCount, int nForceMipCount = -1 ) = 0;

	// Methods to set other texture fields
	virtual void SetBumpScale( float flScale ) = 0;
	virtual void SetReflectivity( const Vector &vecReflectivity ) = 0;

	// Methods to initialize the low-res image
	virtual void InitLowResImage( int nWidth, int nHeight, ImageFormat fmt ) = 0;

	// When unserializing, we can skip a certain number of mip levels,
	// and we also can just load everything but the image data
	// NOTE: If you load only the buffer header, you'll need to use the
	// VTFBufferHeaderSize() method below to only read that much from the file
	// NOTE: If you skip mip levels, the height + width of the texture will
	// change to reflect the size of the largest read in mip level
	virtual bool Unserialize( CUtlBuffer &buf, bool bBufferHeaderOnly = false, int nSkipMipLevels = 0 ) = 0;
	virtual bool Serialize( CUtlBuffer &buf ) = 0;

	// These are methods to help with optimization:
	// Once the header is read in, they indicate where to start reading
	// other data (measured from file start), and how many bytes to read....
	virtual void LowResFileInfo( int *pStartLocation, int *pSizeInBytes) const = 0;
	virtual void ImageFileInfo( int nFrame, int nFace, int nMip, int *pStartLocation, int *pSizeInBytes) const = 0;
	virtual int FileSize( int nMipSkipCount = 0 ) const = 0;

	// Attributes...
	virtual int Width() const = 0;
	virtual int Height() const = 0;
	virtual int MipCount() const = 0;

	// returns the size of one row of a particular mip level
	virtual int RowSizeInBytes( int nMipLevel ) const = 0;

	virtual ImageFormat Format() const = 0;
	virtual int FaceCount() const = 0;
	virtual int FrameCount() const = 0;
	virtual int Flags() const = 0;

	virtual float BumpScale() const = 0;

	virtual int LowResWidth() const = 0;
	virtual int LowResHeight() const = 0;
	virtual ImageFormat LowResFormat() const = 0;

	// NOTE: reflectivity[0] = blue, [1] = greem, [2] = red
	virtual const Vector &Reflectivity() const = 0;

	virtual bool IsCubeMap() const = 0;
	virtual bool IsNormalMap() const = 0;

	// Computes the dimensions of a particular mip level
	virtual void ComputeMipLevelDimensions( int iMipLevel, int *pMipWidth, int *pMipHeight ) const = 0;

	// Computes the size (in bytes) of a single mipmap of a single face of a single frame 
	virtual int ComputeMipSize( int iMipLevel ) const = 0;

	// Computes the size of a subrect (specified at the top mip level) at a particular lower mip level
	virtual void ComputeMipLevelSubRect( Rect_t* pSrcRect, int nMipLevel, Rect_t *pSubRect ) const = 0;

	// Computes the size (in bytes) of a single face of a single frame
	// All mip levels starting at the specified mip level are included
	virtual int ComputeFaceSize( int iStartingMipLevel = 0 ) const = 0;

	// Computes the total size (in bytes) of all faces, all frames
	virtual int ComputeTotalSize() const = 0;

	// Returns the base address of the image data
	virtual unsigned char *ImageData() = 0;

	// Returns a pointer to the data associated with a particular frame, face, and mip level
	virtual unsigned char *ImageData( int iFrame, int iFace, int iMipLevel ) = 0;

	// Returns a pointer to the data associated with a particular frame, face, mip level, and offset
	virtual unsigned char *ImageData( int iFrame, int iFace, int iMipLevel, int x, int y ) = 0;

	// Returns the base address of the low-res image data
	virtual unsigned char *LowResImageData() = 0;

	// Converts the textures image format. Use IMAGE_FORMAT_DEFAULT
	// if you want to be able to use various tool functions below
	virtual	void ConvertImageFormat( ImageFormat fmt, bool bNormalToDUDV ) = 0;

	// NOTE: The following methods only work on textures using the
	// IMAGE_FORMAT_DEFAULT!

	// Generate spheremap based on the current cube faces (only works for cubemaps)
	// The look dir indicates the direction of the center of the sphere
	// NOTE: Only call this *after* cube faces have been correctly
	// oriented (using FixCubemapFaceOrientation)
	virtual void GenerateSpheremap( LookDir_t lookDir = LOOK_DOWN_Z ) = 0;

	// Generate spheremap based on the current cube faces (only works for cubemaps)
	// The look dir indicates the direction of the center of the sphere
	// NOTE: Only call this *after* cube faces have been correctly
	// oriented (using FixCubemapFaceOrientation)
	virtual void GenerateHemisphereMap( unsigned char *pSphereMapBitsRGBA, int targetWidth, 
		int targetHeight, LookDir_t lookDir, int iFrame ) = 0;

	// Fixes the cubemap faces orientation from our standard to the
	// standard the material system needs.
	virtual void FixCubemapFaceOrientation( ) = 0;

	// Generates mipmaps from the base mip levels
	virtual void GenerateMipmaps() = 0;

	// Put 1/miplevel (1..n) into alpha.
	virtual void PutOneOverMipLevelInAlpha() = 0;
	
	// Computes the reflectivity
	virtual void ComputeReflectivity( ) = 0;

	// Computes the alpha flags
	virtual void ComputeAlphaFlags() = 0;

	// Generate the low-res image bits
	virtual bool ConstructLowResImage() = 0;

	// Gets the texture all internally consistent assuming you've loaded
	// mip 0 of all faces of all frames
	virtual void PostProcess(bool bGenerateSpheremap, LookDir_t lookDir = LOOK_DOWN_Z, bool bAllowFixCubemapOrientation = true) = 0;

	// Blends adjacent pixels on cubemap borders, since the card doesn't do it. If the texture
	// is S3TC compressed, then it has to do it AFTER the texture has been compressed to prevent
	// artifacts along the edges.
	//
	// If bSkybox is true, it assumes the faces are oriented in the way the engine draws the skybox
	// (which happens to be different from the way cubemaps have their faces).
	virtual void MatchCubeMapBorders( int iStage, ImageFormat finalFormat, bool bSkybox ) = 0;

	// Sets threshhold values for alphatest mipmapping
	virtual void SetAlphaTestThreshholds( float flBase, float flHighFreq ) = 0;
};


//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------
IVTFTexture *CreateVTFTexture();
void DestroyVTFTexture( IVTFTexture *pTexture );


//-----------------------------------------------------------------------------
// Allows us to only load in the first little bit of the VTF file to get info
// Clients should read this much into a UtlBuffer and then pass it in to
// Unserialize
//-----------------------------------------------------------------------------
int VTFFileHeaderSize();

#endif // VTF_H
