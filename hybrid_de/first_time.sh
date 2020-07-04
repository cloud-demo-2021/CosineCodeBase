curl https://sh.rustup.rs -sSf | sh
export PATH="$HOME/.cargo/bin:$PATH"
source $HOME/.cargo/env
sudo apt install cargo
rustup install nightly
rustup default nightly

g++ -O3 -o workload workload.cc -std=c++11
cd src
cargo build