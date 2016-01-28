; ModuleID = 'CCoscope compiler'

@0 = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @putchar(i32)

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  br label %header

header:                                           ; preds = %body, %entry
  %a_addr.0 = phi i32 [ 10, %entry ], [ %subtmp, %body ], [ undef, <badref> ]
  %cmptmp = icmp eq i32 %a_addr.0, 0
  br i1 %cmptmp, label %postwhile, label %body

body:                                             ; preds = %header
  %subtmp = add i32 %a_addr.0, -1
  %modtmp5 = and i32 %subtmp, 1
  %cmptmp3 = icmp eq i32 %modtmp5, 0
  %calltmp = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @0, i64 0, i64 0), i32 %subtmp)
  br label %header

postwhile:                                        ; preds = %header
  ret i32 0
}
