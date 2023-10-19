import numpy as np
from scipy.optimize import curve_fit

# 数据
data = [
    (81, 3, 960),
    (62, 6, 960),
    (47.5, 10, 960),
    (20.7, 8, 240),
    (35, 8, 480),
    (66, 8, 1440)
]

# 定义关系函数
def func(X, a, b, c, d, e, f):
    n, theta = X
    return a + b * n + c * theta + d * n**2 + e * theta**2 + f * n * theta

# 拟合
initial_guess = [1, 1, 1, 1, 1, 1]  # 初始猜测值
params, covariance = curve_fit(func, (n, theta), v, p0=initial_guess)

# 输出拟合结果
a_fit, b_fit, c_fit, d_fit, e_fit, f_fit = params
print(f"a: {a_fit}, b: {b_fit}, c: {c_fit}, d: {d_fit}, e: {e_fit}, f: {f_fit}")
