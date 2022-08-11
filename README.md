<p>
<p align="center">
  <a href="https://tdengine.com" target="_blank">
  <img
    src="docs/assets/tdengine.svg"
    alt="TDengine"
    width="500"
  />
  </a>
</p>
<p>

[![Build Status](https://cloud.drone.io/api/badges/taosdata/TDengine/status.svg?ref=refs/heads/master)](https://cloud.drone.io/taosdata/TDengine)
[![Build status](https://ci.appveyor.com/api/projects/status/kf3pwh2or5afsgl9/branch/master?svg=true)](https://ci.appveyor.com/project/sangshuduo/tdengine-2n8ge/branch/master)
[![Coverage Status](https://coveralls.io/repos/github/taosdata/TDengine/badge.svg?branch=develop)](https://coveralls.io/github/taosdata/TDengine?branch=develop)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/4201/badge)](https://bestpractices.coreinfrastructure.org/projects/4201)
[![tdengine](https://snapcraft.io//tdengine/badge.svg)](https://snapcraft.io/tdengine)

English | [简体中文](README-CN.md) | We are hiring, check [here](https://tdengine.com/careers)

# What is TDengine？

TDengine is an open source, cloud native time-series database optimized for Internet of Things (IoT), Connected Cars, and Industrial IoT. It enables efficient, real-time data ingestion, processing, and monitoring of TB and even PB scale data per day, generated by billions of sensors and data collectors. Below are the most outstanding advantages of TDengine:

- High-Performance: TDengine is the only time-series database to solve the high cardinality issue to support billions of data collection points while out performing other time-series databases for data ingestion, querying and data compression.

- Simplified Solution: Through built-in caching, stream processing and data subscription features, TDengine provides a simplified solution for time-series data processing. It reduces system design complexity and operation costs significantly.

- Cloud Native: Through native distributed design, sharding and partitioning, separation of compute and storage, RAFT, support for kubernetes deployment and full observability, TDengine can be deployed on public, private or hybrid clouds.

- Open Source: TDengine’s core modules, including cluster feature, are all available under open source licenses. It has gathered 18.7k stars on GitHub, an active developer community, and over 137k running instances worldwide.

- Ease of Use: For administrators, TDengine significantly reduces the effort to deploy and maintain. For developers, it provides a simple interface, simplified solution and seamless integrations for third party tools. For data users, it gives easy data access. 

- Easy Data Analytics: Through super tables, storage and compute separation, data partitioning by time interval, pre-computation and other means, TDengine makes it easy to explore, format, and get access to data in a highly efficient way. 

# Documentation

For user manual, system design and architecture, engineering blogs, refer to [TDengine Documentation](https://docs.tdengine.com)(中文版请点击[这里](https://docs.taosdata.com))

# Building

At the moment, TDengine server only supports running on Linux systems. You can choose to [install from packages](https://www.taosdata.com/en/getting-started/#Install-from-Package) or build it from the source code. This quick guide is for installation from the source only.

To build TDengine, use [CMake](https://cmake.org/) 3.0.2 or higher versions in the project directory.

## Install build dependencies

### Ubuntu 18.04 and above or Debian

```bash
sudo apt-get install -y gcc cmake build-essential git libssl-dev
```

To compile and package the JDBC driver source code, you should have a Java jdk-8 or higher and Apache Maven 2.7 or higher installed.

To install openjdk-8:

```bash
sudo apt-get install -y openjdk-8-jdk
```

To install Apache Maven:

```bash
sudo apt-get install -y maven
```

#### Install build dependencies for taosTools

We provide a few useful tools such as taosBenchmark (was named taosdemo) and taosdump. They were part of TDengine. From TDengine 2.4.0.0, taosBenchmark and taosdump were not released together with TDengine.
By default, TDengine compiling does not include taosTools. You can use 'cmake .. -DBUILD_TOOLS=true' to make them be compiled with TDengine.

To build the [taosTools](https://github.com/taosdata/taos-tools) on Ubuntu/Debian, the following packages need to be installed.

```bash
sudo apt install build-essential libjansson-dev libsnappy-dev liblzma-dev libz-dev pkg-config
```

### CentOS 7.9

```bash
sudo yum install epel-release
sudo yum update
sudo yum install -y gcc gcc-c++ make cmake3 git openssl-devel
sudo ln -sf /usr/bin/cmake3 /usr/bin/cmake
```

To install openjdk-8:

```bash
sudo yum install -y java-1.8.0-openjdk
```

To install Apache Maven:

```bash
sudo yum install -y maven
```

### CentOS 8 & Fedora

```bash
sudo dnf install -y gcc gcc-c++ make cmake epel-release git openssl-devel
```

To install openjdk-8:

```bash
sudo dnf install -y java-1.8.0-openjdk
```

To install Apache Maven:

```bash
sudo dnf install -y maven
```

#### Install build dependencies for taosTools on CentOS

To build the [taosTools](https://github.com/taosdata/taos-tools) on CentOS, the following packages need to be installed.

```bash
sudo yum install zlib-devel xz-devel snappy-devel jansson jansson-devel pkgconfig libatomic libstdc++-static openssl-devel
```

Note: Since snappy lacks pkg-config support (refer to [link](https://github.com/google/snappy/pull/86)), it lead a cmake prompt libsnappy not found. But snappy will works well.

### Setup golang environment

TDengine includes few components developed by Go language. Please refer to golang.org official documentation for golang environment setup.

Please use version 1.14+. For the user in China, we recommend using a proxy to accelerate package downloading.

```
go env -w GO111MODULE=on
go env -w GOPROXY=https://goproxy.cn,direct
```

### Setup rust environment

TDengine includees few compoments developed by Rust language. Please refer to rust-lang.org official documentation for rust environment setup.

## Get the source codes

First of all, you may clone the source codes from github:

```bash
git clone https://github.com/taosdata/TDengine.git
cd TDengine
```

The connectors for go & Grafana and some tools have been moved to separated repositories.

You can modify the file ~/.gitconfig to use ssh protocol instead of https for better download speed. You need to upload ssh public key to GitHub first. Please refer to GitHub official documentation for detail.

```
[url "git@github.com:"]
    insteadOf = https://github.com/
```

## Build TDengine

### On Linux platform

You can run the bash script `build.sh` to build both TDengine and taosTools including taosBenchmark and taosdump as below:

```bash
./build.sh
```

It equals to execute following commands:

```bash
mkdir debug
cd debug
cmake .. -DBUILD_TOOLS=true
make
```

Note TDengine 2.3.x.0 and later use a component named 'taosAdapter' to play http daemon role by default instead of the http daemon embedded in the early version of TDengine. The taosAdapter is programmed by go language. If you pull TDengine source code to the latest from an existing codebase, please execute 'git submodule update --init --recursive' to pull taosAdapter source code. Please install go language version 1.14 or above for compiling taosAdapter. If you meet difficulties regarding 'go mod', especially you are from China, you can use a proxy to solve the problem.

```
go env -w GO111MODULE=on
go env -w GOPROXY=https://goproxy.cn,direct
```

The embedded http daemon still be built from TDengine source code by default. Or you can use the following command to choose to build taosAdapter.

```
cmake .. -DBUILD_HTTP=false
```

You can use Jemalloc as memory allocator instead of glibc:

```
apt install autoconf
cmake .. -DJEMALLOC_ENABLED=true
```

TDengine build script can detect the host machine's architecture on X86-64, X86, arm64 platform.
You can also specify CPUTYPE option like aarch64 too if the detection result is not correct:

aarch64:

```bash
cmake .. -DCPUTYPE=aarch64 && cmake --build .
```

### On Windows platform

If you use the Visual Studio 2013, please open a command window by executing "cmd.exe".
Please specify "amd64" for 64 bits Windows or specify "x86" is for 32 bits Windows when you execute vcvarsall.bat.

```cmd
mkdir debug && cd debug
"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" < amd64 | x86 >
cmake .. -G "NMake Makefiles"
nmake
```

If you use the Visual Studio 2019 or 2017:

please open a command window by executing "cmd.exe".
Please specify "x64" for 64 bits Windows or specify "x86" is for 32 bits Windows when you execute vcvarsall.bat.

```cmd
mkdir debug && cd debug
"c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" < x64 | x86 >
cmake .. -G "NMake Makefiles"
nmake
```

Or, you can simply open a command window by clicking Windows Start -> "Visual Studio < 2019 | 2017 >" folder -> "x64 Native Tools Command Prompt for VS < 2019 | 2017 >" or "x86 Native Tools Command Prompt for VS < 2019 | 2017 >" depends what architecture your Windows is, then execute commands as follows:

```cmd
mkdir debug && cd debug
cmake .. -G "NMake Makefiles"
nmake
```

### On macOS platform

Please install XCode command line tools and cmake. Verified with XCode 11.4+ on Catalina and Big Sur.

```shell
mkdir debug && cd debug
cmake .. && cmake --build .
```

# Installing

## On Linux platform

After building successfully, TDengine can be installed by

```bash
sudo make install
```

Users can find more information about directories installed on the system in the [directory and files](https://www.taosdata.com/en/documentation/administrator/#Directory-and-Files) section. Since version 2.0, installing from source code will also configure service management for TDengine.
Users can also choose to [install from packages](https://www.taosdata.com/en/getting-started/#Install-from-Package) for it.

To start the service after installation, in a terminal, use:

```bash
sudo systemctl start taosd
```

Then users can use the [TDengine shell](https://www.taosdata.com/en/getting-started/#TDengine-Shell) to connect the TDengine server. In a terminal, use:

```bash
taos
```

If TDengine shell connects the server successfully, welcome messages and version info are printed. Otherwise, an error message is shown.

### Install TDengine by apt-get

If you use Debian or Ubuntu system, you can use 'apt-get' command to install TDengine from official repository. Please use following commands to setup:

```
wget -qO - http://repos.taosdata.com/tdengine.key | sudo apt-key add -
echo "deb [arch=amd64] http://repos.taosdata.com/tdengine-stable stable main" | sudo tee /etc/apt/sources.list.d/tdengine-stable.list
[Optional] echo "deb [arch=amd64] http://repos.taosdata.com/tdengine-beta beta main" | sudo tee /etc/apt/sources.list.d/tdengine-beta.list
sudo apt-get update
apt-cache policy tdengine
sudo apt-get install tdengine
```

## On Windows platform

After building successfully, TDengine can be installed by:

```cmd
nmake install
```

## On macOS platform

After building successfully, TDengine can be installed by:

```bash
sudo make install
```

To start the service after installation, config `.plist` file first, in a terminal, use:

```bash
sudo cp ../packaging/macOS/com.taosdata.tdengine.plist /Library/LaunchDaemons
```

To start the service, in a terminal, use:

```bash
sudo launchctl load /Library/LaunchDaemons/com.taosdata.tdengine.plist
```

To stop the service, in a terminal, use:

```bash
sudo launchctl unload /Library/LaunchDaemons/com.taosdata.tdengine.plist
```

## Quick Run

If you don't want to run TDengine as a service, you can run it in current shell. For example, to quickly start a TDengine server after building, run the command below in terminal: (We take Linux as an example, command on Windows will be `taosd.exe`)

```bash
./build/bin/taosd -c test/cfg
```

In another terminal, use the TDengine shell to connect the server:

```bash
./build/bin/taos -c test/cfg
```

option "-c test/cfg" specifies the system configuration file directory.

# Try TDengine

It is easy to run SQL commands from TDengine shell which is the same as other SQL databases.

```sql
CREATE DATABASE demo;
USE demo;
CREATE TABLE t (ts TIMESTAMP, speed INT);
INSERT INTO t VALUES('2019-07-15 00:00:00', 10);
INSERT INTO t VALUES('2019-07-15 01:00:00', 20);
SELECT * FROM t;
          ts          |   speed   |
===================================
 19-07-15 00:00:00.000|         10|
 19-07-15 01:00:00.000|         20|
Query OK, 2 row(s) in set (0.001700s)
```

# Developing with TDengine

## Official Connectors

TDengine provides abundant developing tools for users to develop on TDengine. Follow the links below to find your desired connectors and relevant documentation.

- [Java](https://docs.taosdata.com/reference/connector/java/)
- [C/C++](https://docs.taosdata.com/reference/connector/cpp/)
- [Python](https://docs.taosdata.com/reference/connector/python/)
- [Go](https://docs.taosdata.com/reference/connector/go/)
- [Node.js](https://docs.taosdata.com/reference/connector/node/)
- [Rust](https://docs.taosdata.com/reference/connector/rust/)
- [C#](https://docs.taosdata.com/reference/connector/csharp/)
- [RESTful API](https://docs.taosdata.com/reference/rest-api/)

## Third Party Connectors

The TDengine community has also kindly built some of their own connectors! Follow the links below to find the source code for them.

- [Rust Bindings](https://github.com/songtianyi/tdengine-rust-bindings/tree/master/examples)
- [.Net Core Connector](https://github.com/maikebing/Maikebing.EntityFrameworkCore.Taos)
- [Lua Connector](https://github.com/taosdata/TDengine/tree/develop/tests/examples/lua)
- [PHP](https://www.taosdata.com/en/documentation/connector#c-cpp)

# How to run the test cases and how to add a new test case

TDengine's test framework and all test cases are fully open source.
Please refer to [this document](https://github.com/taosdata/TDengine/blob/develop/tests/How-To-Run-Test-And-How-To-Add-New-Test-Case.md) for how to run test and develop new test case.

# Contribute to TDengine

Please follow the [contribution guidelines](CONTRIBUTING.md) to contribute to the project.

# Join TDengine WeChat Group

Add WeChat “tdengine” to join the group，you can communicate with other users.

# [User List](https://github.com/taosdata/TDengine/issues/2432)

If you are using TDengine and feel it helps or you'd like to do some contributions, please add your company to [user list](https://github.com/taosdata/TDengine/issues/2432) and let us know your needs.
