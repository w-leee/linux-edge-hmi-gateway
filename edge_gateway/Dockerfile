# ==========================================
# 阶段一：编译阶段 (Builder)
# ==========================================
FROM ubuntu:22.04 AS builder

# 换阿里云源
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list && \
    sed -i 's@//.*security.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list

# 安装编译工具和依赖
RUN apt-get update && apt-get install -y \
    build-essential cmake \
    libspdlog-dev libyaml-cpp-dev libmosquitto-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
# 把源码拷进来
COPY . .

# 编译
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j4

# ==========================================
# 阶段二：运行阶段 (Runner)
# ==========================================
FROM ubuntu:22.04

# 换阿里云源
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list && \
    sed -i 's@//.*security.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list

# 只安装运行所需的动态链接库
RUN apt-get update && apt-get install -y \
    libspdlog1 libyaml-cpp0.7 libmosquitto1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/gateway .
COPY --from=builder /app/config.yaml .

EXPOSE 8080
CMD ["./gateway"]
