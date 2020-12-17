


varying lowp vec2 varyTextCoord;
precision mediump float;
uniform sampler2D lumaTexture;
uniform sampler2D chromTexture;
uniform mat3 yuvToRgbMatrix;

void main()
{
    mediump vec3 yuv;
    mediump vec3 rgb;
    
    yuv.x = texture2D(lumaTexture,varyTextCoord).r;
    
    yuv.yz = texture2D(chromTexture,varyTextCoord).ra - vec2(0.5,0.5);
    
    rgb = yuvToRgbMatrix * yuv;
    
    gl_FragColor = vec4(rgb,1.0);
    
}
