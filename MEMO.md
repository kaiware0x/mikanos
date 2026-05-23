# Study MikanOS

- https://github.com/kaiware0x/mikanos
  - OSのソースコードがある
  - 勉強用写経リポジトリ
    - mydevelop branch
- https://github.com/kaiware0x/mikanos-build
  - OSビルドのための便利スクリプトなどがある
  - Python3.12 では distutils は削除されているので ansible_provision.yml からも削除した。
    - -> 環境構築で躓きたくないので、Ubuntu20を使うことにし、 `ansible_provision.yml` の修正の必要もなくなった
- Oh-my-posh のテーマを更新。 pure というやつが見やすそうだった。
- EDK II ライブラリについて
  - UEFI アプリケーションを作成するためのオープンソースライブラリ
  - 利用する前には `~/edk2/edksetup.sh` というスクリプトを実行しておく必要がある
  - 含まれるパッケージ:
    - `MdePkg`: 基本ライブラリ
    - `AppPkg`: 様々な UEFI アプリケーションのサンプル集
    - `OvmfPkg`: UEFI BIOS のオープンソース実装である OVMF が収められている
- `MikanLoaderPkg`
  - メインメモリに OS を読み込むためのブートローダー
  - `Loader.inf`
    - コンポーネント定義ファイル
    - ブートローダーの設定を記載する
  - `.dec`
    - パッケージ宣言ファイル
  - `.dsc`
    - パッケージ記述ファイル

## BootLoader のビルド

```sh
cd ~/edk2
# シンボリックリンクを張っておく
ln -s $HOME/workspace/mikanos/MikanLoaderPkg ./
source edksetup.sh
build
```

ビルド前に `Conf/target.txt` を以下のように編集しておく。

| 設定項目        | 設定値                            |
| --------------- | --------------------------------- |
| ACTIVE_PLATFORM | MikanLoaderPkg/MikanLoaderPkg.dsc |
| TARGET          | DEBUG                             |
| TARGET_ARCH     | X64                               |
| TOOL_CHAIN_TAG  | CLANG38                           |


## QEMU で EFI アプリケーションを実行

```sh
$HOME/osbook/devenv/run_qemu.sh $HOME/edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi

# kernel プログラムを渡す場合
$HOME/osbook/devenv/run_qemu.sh $HOME/edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi $HOME/workspace/mikanos/kernel/kernel.elf
```

## disk.img の中身の確認

EFIアプリケーションを実行したカレントディレクトリに `disk.img` というファイルができる。
これは USB メモリの中身を1つに固めたファイルで、マウントして中身を見れる。

```sh
mkdir -p mnt
sudo mount -o loop disk.img mnt
ls mnt
sudo umount mnt
```

> ブートローダはUEFI アプリとして、カーネルはELF バイナリとして別々のファイルとして開発し、ブートローダからカーネルを呼び出す形式にしようと思います。

## カーネルのビルド

```sh
cd ~/workspace/mikanos/kernel

# freestanding: OS が無い環境で動くプログラム向け
clang++ -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17 -c main.cpp

ld.lld --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o
```

## コラム 3.1 レッドゾーン

> レッドゾーン（red zone）はスタックポインタを少し超えたスタック領域のことです。
> System V AMD64 ABI（参考文献［6］）ではRSPの手前128バイトの領域がレッドゾーンと規定されています。
> この領域は実行中の関数により予約されていて、割り込みハンドラが勝手に変更してはならない、と決められています

他の関数を呼び出さない関数（leaf function）はRSP の値を調整することなくレッ
ドゾーンを使って良い

## 3.4

> フレームバッファ（Frame Buffer）とはピクセル（に描画するための値）を敷き詰めたメモリ領域のことです。
> フレームバッファの各点に値を書き込むと、それがディスプレイのピクセルに反映される仕組みになっています。
