#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <render_backend.h>
#include "d3d11scene.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {

const UINT kWidth = 320;
const UINT kHeight = 180;

template <class T> void Release(T *&object) {
  if (object)
    object->Release();
  object = NULL;
}

struct SceneConstants {
  float width, height, fogEnabled, useSecondTexture;
  float fogColor[4];
  float alphaTest, padding[3];
};

#pragma pack(push, 1)
struct BitmapHeader {
  uint16_t type;
  uint32_t size;
  uint16_t reserved1, reserved2;
  uint32_t pixelOffset;
  uint32_t infoSize;
  int32_t width, height;
  uint16_t planes, bitsPerPixel;
  uint32_t compression, imageSize;
  int32_t xPixelsPerMeter, yPixelsPerMeter;
  uint32_t colorsUsed, colorsImportant;
};
#pragma pack(pop)

static HRESULT Compile(const char *source, const char *target, ID3DBlob **blob) {
  ID3DBlob *errors = NULL;
  HRESULT hr = D3DCompile(source, strlen(source), "d3d11scene.h", NULL, NULL,
                          "main", target, D3DCOMPILE_ENABLE_STRICTNESS, 0,
                          blob, &errors);
  if (FAILED(hr) && errors)
    fprintf(stderr, "%s\n", (const char *)errors->GetBufferPointer());
  Release(errors);
  return hr;
}

static ID3D11ShaderResourceView *CreateColorTexture(ID3D11Device *device,
                                                     uint8_t r, uint8_t g,
                                                     uint8_t b, uint8_t a) {
  uint8_t pixel[4] = {r, g, b, a};
  D3D11_TEXTURE2D_DESC desc = {};
  D3D11_SUBRESOURCE_DATA data = {};
  ID3D11Texture2D *texture = NULL;
  ID3D11ShaderResourceView *view = NULL;
  desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  data.pSysMem = pixel;
  data.SysMemPitch = 4;
  if (FAILED(device->CreateTexture2D(&desc, &data, &texture)) ||
      FAILED(device->CreateShaderResourceView(texture, NULL, &view)))
    Release(view);
  Release(texture);
  return view;
}

static sRenderBackendVertex Vertex(float x, float y, float r, float g, float b,
                                   float a) {
  sRenderBackendVertex vertex = {};
  vertex.x = x;
  vertex.y = y;
  vertex.z = 0.5f;
  vertex.rhw = 1.0f;
  vertex.r = r;
  vertex.g = g;
  vertex.b = b;
  vertex.a = a;
  vertex.fog = 1.0f;
  return vertex;
}

static void DrawRect(ID3D11Device *device, ID3D11DeviceContext *context,
                     float left, float top, float right, float bottom,
                     float r, float g, float b, float topAlpha,
                     float bottomAlpha) {
  sRenderBackendVertex vertices[6] = {
      Vertex(left, top, r, g, b, topAlpha),
      Vertex(right, top, r, g, b, topAlpha),
      Vertex(right, bottom, r, g, b, bottomAlpha),
      Vertex(left, top, r, g, b, topAlpha),
      Vertex(right, bottom, r, g, b, bottomAlpha),
      Vertex(left, bottom, r, g, b, bottomAlpha),
  };
  D3D11_BUFFER_DESC desc = {};
  D3D11_SUBRESOURCE_DATA data = {};
  ID3D11Buffer *buffer = NULL;
  UINT stride = sizeof(vertices[0]), offset = 0;
  desc.ByteWidth = sizeof(vertices);
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  data.pSysMem = vertices;
  if (FAILED(device->CreateBuffer(&desc, &data, &buffer)))
    return;
  context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
  context->Draw(6, 0);
  Release(buffer);
}

static bool Near(uint8_t actual, float expected) {
  int wanted = (int)(expected * 255.0f + 0.5f);
  int delta = (int)actual - wanted;
  return delta >= -2 && delta <= 2;
}

static bool CheckPixel(const uint8_t *pixels, UINT pitch, UINT x, UINT y,
                       float r, float g, float b, const char *label) {
  const uint8_t *pixel = pixels + y * pitch + x * 4;
  bool pass = Near(pixel[0], r) && Near(pixel[1], g) && Near(pixel[2], b);
  printf("%-28s (%3u,%3u) actual=%3u,%3u,%3u expected=%3d,%3d,%3d %s\n",
         label, x, y, pixel[0], pixel[1], pixel[2],
         (int)(r * 255 + .5f), (int)(g * 255 + .5f),
         (int)(b * 255 + .5f), pass ? "PASS" : "FAIL");
  return pass;
}

static bool WriteBmp(const char *path, const uint8_t *pixels, UINT pitch) {
  FILE *file = NULL;
  BitmapHeader header = {};
  if (fopen_s(&file, path, "wb") != 0)
    return false;
  header.type = 0x4d42;
  header.pixelOffset = sizeof(header);
  header.infoSize = 40;
  header.width = kWidth;
  header.height = -(int32_t)kHeight;
  header.planes = 1;
  header.bitsPerPixel = 32;
  header.imageSize = kWidth * kHeight * 4;
  header.size = header.pixelOffset + header.imageSize;
  fwrite(&header, sizeof(header), 1, file);
  for (UINT y = 0; y < kHeight; ++y) {
    for (UINT x = 0; x < kWidth; ++x) {
      const uint8_t *source = pixels + y * pitch + x * 4;
      uint8_t bgra[4] = {source[2], source[1], source[0], source[3]};
      fwrite(bgra, sizeof(bgra), 1, file);
    }
  }
  fclose(file);
  return true;
}

} // namespace

int main(int argc, char **argv) {
  const char *output = argc > 1 ? argv[1] : "render_regression.bmp";
  ID3D11Device *device = NULL;
  ID3D11DeviceContext *context = NULL;
  ID3D11Texture2D *target = NULL, *staging = NULL;
  ID3D11RenderTargetView *targetView = NULL;
  ID3D11VertexShader *vertexShader = NULL;
  ID3D11PixelShader *pixelShader = NULL;
  ID3D11InputLayout *layout = NULL;
  ID3D11Buffer *constantBuffer = NULL;
  ID3D11SamplerState *sampler = NULL;
  ID3D11BlendState *opaque = NULL, *alpha = NULL;
  ID3D11ShaderResourceView *white = NULL, *alpha0 = NULL, *alphaHalf = NULL,
                           *alphaOne = NULL;
  ID3DBlob *vsBlob = NULL, *psBlob = NULL;
  D3D_FEATURE_LEVEL level;
  HRESULT hr;
  int result = 1;

  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = kWidth;
  textureDesc.Height = kHeight;
  textureDesc.MipLevels = textureDesc.ArraySize = 1;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
  hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, NULL, 0,
                         D3D11_SDK_VERSION, &device, &level, &context);
  if (FAILED(hr) || FAILED(device->CreateTexture2D(&textureDesc, NULL, &target)) ||
      FAILED(device->CreateRenderTargetView(target, NULL, &targetView)))
    goto cleanup;

  textureDesc.Usage = D3D11_USAGE_STAGING;
  textureDesc.BindFlags = 0;
  textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  if (FAILED(device->CreateTexture2D(&textureDesc, NULL, &staging)) ||
      FAILED(Compile(kD3D11SceneVS, "vs_4_0", &vsBlob)) ||
      FAILED(Compile(kD3D11ScenePS, "ps_4_0", &psBlob)) ||
      FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(),
                                        vsBlob->GetBufferSize(), NULL,
                                        &vertexShader)) ||
      FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(),
                                       psBlob->GetBufferSize(), NULL,
                                       &pixelShader)))
    goto cleanup;

  {
    D3D11_INPUT_ELEMENT_DESC elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"FOG", 0, DXGI_FORMAT_R32_FLOAT, 0, 32,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 44,
         D3D11_INPUT_PER_VERTEX_DATA, 0}};
    if (FAILED(device->CreateInputLayout(elements, 5, vsBlob->GetBufferPointer(),
                                         vsBlob->GetBufferSize(), &layout)))
      goto cleanup;
  }
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(SceneConstants);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&desc, NULL, &constantBuffer)))
      goto cleanup;
  }
  {
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&desc, &sampler)))
      goto cleanup;
  }
  {
    D3D11_BLEND_DESC desc;
    D3D11DescribeBlend(kRenderBackendBlendOpaque, &desc);
    if (FAILED(device->CreateBlendState(&desc, &opaque)))
      goto cleanup;
    D3D11DescribeBlend(kRenderBackendBlendAlpha, &desc);
    if (FAILED(device->CreateBlendState(&desc, &alpha)))
      goto cleanup;
  }
  white = CreateColorTexture(device, 255, 255, 255, 255);
  alpha0 = CreateColorTexture(device, 255, 255, 255, 0);
  alphaHalf = CreateColorTexture(device, 255, 255, 255, 128);
  alphaOne = CreateColorTexture(device, 255, 255, 255, 255);
  if (!white || !alpha0 || !alphaHalf || !alphaOne)
    goto cleanup;

  {
    float clear[4] = {16 / 255.0f, 28 / 255.0f, 64 / 255.0f, 1};
    float blendFactor[4] = {};
    D3D11_VIEWPORT viewport = {};
    SceneConstants constants = {};
    viewport.Width = (float)kWidth;
    viewport.Height = (float)kHeight;
    viewport.MaxDepth = 1;
    constants.width = (float)kWidth;
    constants.height = (float)kHeight;
    context->ClearRenderTargetView(targetView, clear);
    context->OMSetRenderTargets(1, &targetView, NULL);
    context->RSSetViewports(1, &viewport);
    context->IASetInputLayout(layout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertexShader, NULL, 0);
    context->PSSetShader(pixelShader, NULL, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetConstantBuffers(0, 1, &constantBuffer);
    context->UpdateSubresource(constantBuffer, 0, NULL, &constants, 0, 0);
    context->PSSetSamplers(0, 1, &sampler);

    context->OMSetBlendState(opaque, blendFactor, ~0u);
    context->PSSetShaderResources(0, 1, &white);
    DrawRect(device, context, 40, 20, 280, 160, .95f, .85f, .35f, 1, 1);

    context->OMSetBlendState(alpha, blendFactor, ~0u);
    context->PSSetShaderResources(0, 1, &alpha0);
    DrawRect(device, context, 0, 0, 106, 90, .10f, .08f, .05f, .5f, .5f);
    context->PSSetShaderResources(0, 1, &alphaHalf);
    DrawRect(device, context, 106, 0, 213, 90, .10f, .08f, .05f, .5f, .5f);
    context->PSSetShaderResources(0, 1, &alphaOne);
    DrawRect(device, context, 213, 0, 320, 90, .10f, .08f, .05f, .5f, .5f);
    DrawRect(device, context, 0, 90, 320, 180, .10f, .08f, .05f, 0, .5f);
  }

  context->CopyResource(staging, target);
  {
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)))
      goto cleanup;
    const float moon[3] = {.95f, .85f, .35f};
    const float cloud[3] = {.10f, .08f, .05f};
    bool pass = true;
    pass &= CheckPixel((const uint8_t *)mapped.pData, mapped.RowPitch, 80, 45,
                       moon[0], moon[1], moon[2], "texture alpha 0");
    float halfTextureAlpha = .5f * (128 / 255.0f);
    pass &= CheckPixel(
        (const uint8_t *)mapped.pData, mapped.RowPitch, 160, 45,
        cloud[0] * halfTextureAlpha + moon[0] * (1 - halfTextureAlpha),
        cloud[1] * halfTextureAlpha + moon[1] * (1 - halfTextureAlpha),
        cloud[2] * halfTextureAlpha + moon[2] * (1 - halfTextureAlpha),
        "texture .5 * vertex .5");
    pass &= CheckPixel((const uint8_t *)mapped.pData, mapped.RowPitch, 250, 45,
                       cloud[0] * .5f + moon[0] * .5f,
                       cloud[1] * .5f + moon[1] * .5f,
                       cloud[2] * .5f + moon[2] * .5f,
                       "texture 1 * vertex .5");
    float interpolatedAlpha = ((170.5f - 90.0f) / 90.0f) * .5f;
    pass &= CheckPixel(
        (const uint8_t *)mapped.pData, mapped.RowPitch, 160, 170,
        cloud[0] * interpolatedAlpha + (16 / 255.0f) * (1 - interpolatedAlpha),
        cloud[1] * interpolatedAlpha + (28 / 255.0f) * (1 - interpolatedAlpha),
        cloud[2] * interpolatedAlpha + (64 / 255.0f) * (1 - interpolatedAlpha),
        "interpolated vertex alpha");
    if (!WriteBmp(output, (const uint8_t *)mapped.pData, mapped.RowPitch))
      pass = false;
    context->Unmap(staging, 0);
    printf("output: %s\n%s\n", output, pass ? "PASS" : "FAIL");
    result = pass ? 0 : 2;
  }

cleanup:
  Release(alphaOne);
  Release(alphaHalf);
  Release(alpha0);
  Release(white);
  Release(alpha);
  Release(opaque);
  Release(sampler);
  Release(constantBuffer);
  Release(layout);
  Release(pixelShader);
  Release(vertexShader);
  Release(psBlob);
  Release(vsBlob);
  Release(targetView);
  Release(staging);
  Release(target);
  Release(context);
  Release(device);
  if (result == 1)
    fprintf(stderr, "render setup failed (HRESULT 0x%08lx)\n", hr);
  return result;
}
