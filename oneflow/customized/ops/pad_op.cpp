#include "oneflow/core/framework/framework.h"
#include "oneflow/core/common/balanced_splitter.h"

namespace oneflow {

REGISTER_USER_OP("pad")
    .Input("x")
    .Output("y")
    .Attr("padding_before", UserOpAttrType::kAtListInt64)
    .Attr("padding_after", UserOpAttrType::kAtListInt64)
    .Attr("floating_constant_value", UserOpAttrType::kAtDouble)
    .Attr("integral_constant_value", UserOpAttrType::kAtInt64)
    .SetTensorDescInferFn([](user_op::InferContext* ctx) -> Maybe<void> {
      Shape* x_shape = ctx->Shape4ArgNameAndIndex("x", 0);
      auto padding_before = ctx->GetAttr<std::vector<int64_t>>("padding_before");
      auto padding_after = ctx->GetAttr<std::vector<int64_t>>("padding_after");
      CHECK_EQ(padding_before.size(), x_shape->NumAxes());
      CHECK_EQ(padding_after.size(), x_shape->NumAxes());
      DimVector y_dim_vec(x_shape->NumAxes());
      FOR_RANGE(int64_t, i, 0, x_shape->NumAxes()) {
        y_dim_vec[i] = x_shape->At(i) + padding_before[i] + padding_after[i];
      }
      *ctx->Shape4ArgNameAndIndex("y", 0) = Shape(y_dim_vec);
      *ctx->Dtype4ArgNameAndIndex("y", 0) = *ctx->Dtype4ArgNameAndIndex("x", 0);
      return Maybe<void>::Ok();
    })
    .SetGetSbpFn([](user_op::SbpContext* ctx) -> Maybe<void> {
      const user_op::TensorDesc& x_tensor = ctx->LogicalTensorDesc4InputArgNameAndIndex("x", 0);
      auto padding_before = ctx->GetAttr<std::vector<int64_t>>("padding_before");
      auto padding_after = ctx->GetAttr<std::vector<int64_t>>("padding_after");
      for (int64_t i = 0; i < x_tensor.shape().NumAxes(); i++) {
        if (padding_before[i] == 0 && padding_after[i] == 0){
          SbpSignatureBuilder()
              .Split("dx", 0, i)
              .Split("dy", 0, i)
              .Build(ctx->sbp_sig_list()->mutable_sbp_signature()->Add());
        }
      }
      return Maybe<void>::Ok();
    });

REGISTER_USER_OP("pad_grad")
    .Input("dy")
    .Output("dx")
    .Attr("padding_before", UserOpAttrType::kAtListInt64)
    .Attr("padding_after", UserOpAttrType::kAtListInt64)
    .Attr("floating_constant_value", UserOpAttrType::kAtDouble)
    .Attr("integral_constant_value", UserOpAttrType::kAtInt64)
    .SetTensorDescInferFn([](user_op::InferContext* ctx) -> Maybe<void> {
      Shape* dy_shape = ctx->Shape4ArgNameAndIndex("dy", 0);
      auto padding_before = ctx->GetAttr<std::vector<int64_t>>("padding_before");
      auto padding_after = ctx->GetAttr<std::vector<int64_t>>("padding_after");
      CHECK_EQ(padding_before.size(), dy_shape->NumAxes());
      CHECK_EQ(padding_after.size(), dy_shape->NumAxes());
      DimVector dx_dim_vec(dy_shape->NumAxes());
      FOR_RANGE(int64_t, i, 0, dy_shape->NumAxes()) {
        dx_dim_vec[i] = dy_shape->At(i) - padding_before[i] - padding_after[i];
      }
      *ctx->Shape4ArgNameAndIndex("dx", 0) = Shape(dx_dim_vec);
      *ctx->Dtype4ArgNameAndIndex("dx", 0) = *ctx->Dtype4ArgNameAndIndex("dy", 0);
      return Maybe<void>::Ok();
    })
    .SetGetSbpFn([](user_op::SbpContext* ctx) -> Maybe<void> {
      const user_op::TensorDesc& dy_tensor = ctx->LogicalTensorDesc4InputArgNameAndIndex("dy", 0);
      auto padding_before = ctx->GetAttr<std::vector<int64_t>>("padding_before");
      auto padding_after = ctx->GetAttr<std::vector<int64_t>>("padding_after");
      for (int64_t i = 0; i < dy_tensor.shape().NumAxes(); i++) {
        if (padding_before[i] == 0 && padding_after[i] == 0){
          SbpSignatureBuilder()
              .Split("dx", 0, i)
              .Split("dy", 0, i)
              .Build(ctx->sbp_sig_list()->mutable_sbp_signature()->Add());
        }
      }
      return Maybe<void>::Ok();
    });

REGISTER_USER_OP_GRAD("pad").SetGenBackwardOpConfFn([](const user_op::UserOpWrapper& op,
                                                       user_op::AddOpFn AddOp) {
  if (op.NeedGenGradTensor4OpInput("x", 0)) {
    user_op::UserOpConfWrapperBuilder builder(op.op_name() + "_grad");
    user_op::UserOpConfWrapper grad_op =
        builder.Op("pad_grad")
            .Input("dy", op.GetGradTensorWithOpOutput("y", 0))
            .Output("dx")
            .Attr("floating_constant_value", op.attr<double>("floating_constant_value"))
            .Attr("integral_constant_value", op.attr<int64_t>("integral_constant_value"))
            .Attr("padding_before", op.attr<std::vector<int64_t>>("padding_before"))
            .Attr("padding_after", op.attr<std::vector<int64_t>>("padding_after"))
            .Build();
    op.BindGradTensorWithOpInput(grad_op.output("dx", 0), "x", 0);
    AddOp(grad_op);
  }
});

REGISTER_USER_OP_GRAD("pad_grad")
    .SetGenBackwardOpConfFn([](const user_op::UserOpWrapper& op, user_op::AddOpFn AddOp) {
      if (op.NeedGenGradTensor4OpInput("x", 0)) {
        user_op::UserOpConfWrapperBuilder builder(op.op_name() + "_grad");
        user_op::UserOpConfWrapper grad_op =
            builder.Op("pad")
                .Input("x", op.GetGradTensorWithOpOutput("dy", 0))
                .Output("y")
                .Attr("floating_constant_value", op.attr<double>("floating_constant_value"))
                .Attr("integral_constant_value", op.attr<int64_t>("integral_constant_value"))
                .Attr("padding_before", op.attr<std::vector<int64_t>>("padding_before"))
                .Attr("padding_after", op.attr<std::vector<int64_t>>("padding_after"))
                .Build();
        op.BindGradTensorWithOpInput(grad_op.output("y", 0), "dx", 0);
        AddOp(grad_op);
      }
    });

}  // namespace oneflow
