//
//  CCView.h
//  001--GLSL
//
//  Created by CC老师 on 2017/12/16.
//  Copyright © 2017年 CC老师. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef enum {
    NoRotation, // 未旋转
    RotateLeft, // 往左旋转
    RotateRight,    // 往右旋转
    FlipVertical,   // 绕Y轴反转
    FlipHorizonal,  // 绕X轴反转
    RotateRightFlipVertical,    // 右旋，再绕Y轴反转
    RotateRightFlipHorizontal,  // 右旋，再绕X轴反转
    Rotate180   // 180度旋转
} RotationMode;


@interface CCView : UIView

@property (nonatomic , assign) BOOL isFullYUVRange;
- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer;
@end
