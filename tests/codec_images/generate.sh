#!/bin/sh

# Generate reference images.
# The tool `texconv` is from DirectXUtils. It is used as the reference codec.
# Note that it must be compiled with OpenEXR support!
SRCDIR='source'
COMDIR='compressed'
REFDIR='reference'
# Sadly, the texconv API doesn't let us name output directly. So we'll generate
# this and then rename.
PREFIX_COM='COM-'
PREFIX_REF='REF-'

# these can be UNORM or UNORM_SRGB
ALGOS_INT='BC1 BC2 BC3 BC7'
# these can be UNORM or SNORM
ALGOS_RGTC='BC4 BC5'
# these can be UFLOAT or SFLOAT
ALGOS_HDR='BC6H'

mkdir -p "$COMDIR" "$REFDIR"

#texconv_and_mv $name $tag $refformat $comformat $otherflags...
texconv_and_mv() {
    local name="$1"
    local tag="$2"
    local comformat="$3"
    local refformat="$4"
    shift
    shift
    shift
    shift

    # strip `.src.*`
    local bname="$(basename "$name" | sed 's/\(.*\)\.[^.]*$/\1/')"
    local sdir="$(dirname "$name")"
    local pwd="$PWD"

    # note that simply using `-sx` won't work, because we want a lowercase extension!

    cd "$sdir"
    # generate compressed
    texconv -px "$PREFIX_COM" -sx ".$tag" -f "$comformat" -o "$pwd/$COMDIR" -y "$@" "$(basename "$name")"
    cd "$pwd/$COMDIR"
    mv "$PREFIX_COM$bname.$tag.DDS" "$bname.$tag.dds"
    # generate decompressed
    texconv -px "$PREFIX_REF" -f "$refformat" -o "$pwd/$REFDIR" -y "$@" "$bname.$tag.dds"
    cd "$pwd/$REFDIR"
    mv "$PREFIX_REF$bname.$tag.DDS" "$bname.$tag.dds"
    cd "$pwd"
}

# INT & RGTC
for img in ls "$SRCDIR"/*.png; do
    if [ ! -f "$img" ]; then
        continue
    fi
    for algo in $ALGOS_INT; do
        btag="$(printf '%s' "$algo" | tr 'A-Z' 'a-z')"
        texconv_and_mv "$img" "${btag}" "${algo}_UNORM" "R8G8B8A8_UNORM" -srgbi -srgbo &
        texconv_and_mv "$img" "${btag}-srgb" "${algo}_UNORM_SRGB" "R8G8B8A8_UNORM_SRGB" -srgbi -srgbo &
    done
    for algo in $ALGOS_RGTC; do
        btag="$(printf '%s' "$algo" | tr 'A-Z' 'a-z')"
        if [ "$algo" = "BC4" ]; then
            btype='R8'
        elif [ "$algo" = "BC5" ]; then
            btype='R8G8'
        else
            echo "Error: Unknown RGTC algorithm '$algo'"
            exit 1
        fi
        texconv_and_mv "$img" "${btag}-u" "${algo}_UNORM" "${btype}_UNORM" -srgbi -srgbo &
        texconv_and_mv "$img" "${btag}-s" "${algo}_SNORM" "${btype}_SNORM" -srgbi -srgbo &
    done
done
# HDR
for img in "$SRCDIR"/*.exr; do
    if [ ! -f "$img" ]; then
        continue
    fi
    for algo in $ALGOS_HDR; do
        btag="$(printf '%s' "$algo" | tr 'A-Z' 'a-z')"
        echo $btag
        texconv_and_mv "$img" "${btag}-u" "${algo}_UF16" "R16G16B16A16_FLOAT" &
        texconv_and_mv "$img" "${btag}-s" "${algo}_SF16" "R16G16B16A16_FLOAT" &
    done
done
wait
