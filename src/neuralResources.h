#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

class NeuralBinaryData {
public:
    struct TensorInfo {
        std::string name;
        std::vector<uint32_t> dims;   // 按 [N,C,H,W] 或 [Out,In] 等顺序存储
        uint64_t offset = 0;          // 在 m_data 数组中的浮点数偏移（非字节）
        size_t numElements = 0;
    };

    NeuralBinaryData() = default;

    // 禁止拷贝，允许移动
    NeuralBinaryData(const NeuralBinaryData&) = delete;
    NeuralBinaryData& operator=(const NeuralBinaryData&) = delete;
    NeuralBinaryData(NeuralBinaryData&&) = default;
    NeuralBinaryData& operator=(NeuralBinaryData&&) = default;

    // 从文件加载所有权重数据，失败时抛出异常
    void load(const std::string& filepath);

    // 获取所有张量的元数据列表
    const std::vector<TensorInfo>& getTensors() const {
        return m_tensors;
    }

    // 根据名称查找张量信息，找不到返回 nullptr
    const TensorInfo* findTensor(const std::string& name) const {
        for (const auto& t : m_tensors) {
            if (t.name == name) return &t;
        }
        return nullptr;
    }

    // 获取指向整个数据缓冲区的指针和总浮点数，方便一次性创建 Vulkan Storage Buffer
    const float* getDataPtr() const { return m_data.data(); }
    size_t getTotalFloatCount() const { return m_data.size(); }
    size_t getTotalBytes() const { return m_data.size() * sizeof(float); }

    // 获取指定张量的数据指针（CPU 端）
    const float* getTensorData(const std::string& name) const {
        const TensorInfo* info = findTensor(name);
        if (!info) return nullptr;
        return m_data.data() + info->offset;
    }

    // 获取整个连续数据缓冲区的引用（可用于直接上传 GPU）
    const std::vector<float>& dataBuffer() const { return m_data; }

private:
    std::vector<TensorInfo> m_tensors;
    std::vector<float> m_data;
};

// tex.tex torch.Size([1, 7, 400, 400])
// tex.levels.0.tex torch.Size([1, 7, 400, 400])
// tex.levels.1.tex torch.Size([1, 7, 200, 200])
// tex.levels.2.tex torch.Size([1, 7, 100, 100])
// tex.levels.3.tex torch.Size([1, 7, 50, 50])
// off_tex.tex torch.Size([1, 7, 400, 400])
// off_tex.levels.0.tex torch.Size([1, 7, 400, 400])
// off_tex.levels.1.tex torch.Size([1, 7, 200, 200])
// off_tex.levels.2.tex torch.Size([1, 7, 100, 100])
// off_tex.levels.3.tex torch.Size([1, 7, 50, 50])
// net.net.0.linear.weight torch.Size([32, 34])
// net.net.0.linear.bias torch.Size([32])
// net.net.1.linear.weight torch.Size([32, 32])
// net.net.1.linear.bias torch.Size([32])
// net.net.2.linear.weight torch.Size([32, 32])
// net.net.2.linear.bias torch.Size([32])
// net.net.3.weight torch.Size([3, 32])
// net.net.3.bias torch.Size([3])
// off_net.net.0.linear.weight torch.Size([32, 31])
// off_net.net.0.linear.bias torch.Size([32])
// off_net.net.1.linear.weight torch.Size([32, 32])
// off_net.net.1.linear.bias torch.Size([32])
// off_net.net.2.linear.weight torch.Size([32, 32])
// off_net.net.2.linear.bias torch.Size([32])
// off_net.net.3.weight torch.Size([1, 32])
// off_net.net.3.bias torch.Size([1])


// class SineLayer(nn.Module):    
//     def __init__(self, in_features, out_features, bias=True,
//                  is_first=False, omega_0=30):
//         super().__init__()
//         self.omega_0 = omega_0
//         self.is_first = is_first
        
//         self.in_features = in_features
//         self.linear = nn.Linear(in_features, out_features, bias=bias)
        
//         self.init_weights()
    
//     def init_weights(self):
//         with torch.no_grad():
//             if self.is_first:
//                 self.linear.weight.uniform_(-1 / self.in_features, 
//                                              1 / self.in_features)      
//             else:
//                 self.linear.weight.uniform_(-np.sqrt(6 / self.in_features) / self.omega_0, 
//                                              np.sqrt(6 / self.in_features) / self.omega_0)
        
//     def forward(self, x):
//         return torch.sin(self.omega_0 * self.linear(x))

// class Siren(nn.Module):
//     def __init__(self, in_features, hidden_features, hidden_layers, out_features, outermost_linear=False, 
//                  first_omega_0=30, hidden_omega_0=30.):
//         super().__init__()
        
//         self.net = []
//         self.net.append(SineLayer(in_features, hidden_features, 
//                                   is_first=True, omega_0=first_omega_0))

//         for i in range(hidden_layers):
//             self.net.append(SineLayer(hidden_features, hidden_features, 
//                                       is_first=False, omega_0=hidden_omega_0))

//         if outermost_linear:
//             final_linear = nn.Linear(hidden_features, out_features)
            
//             with torch.no_grad():
//                 final_linear.weight.uniform_(-np.sqrt(6 / hidden_features) / hidden_omega_0, 
//                                               np.sqrt(6 / hidden_features) / hidden_omega_0)
                
//             self.net.append(final_linear)
//         else:
//             self.net.append(SineLayer(hidden_features, out_features, 
//                                       is_first=False, omega_0=hidden_omega_0))
        
//         self.net = nn.Sequential(*self.net)
    
//     def forward(self, x):
//         return self.net(x)

// class MLP(nn.Module):
//     def __init__(self, in_features, hidden_features, hidden_layers, out_features, outermost_linear=False):
//         super().__init__()
        
//         self.net = []
//         self.net.append(nn.Linear(in_features, hidden_features))
//         self.net.append(nn.ReLU())

//         for i in range(hidden_layers):
//             self.net.append(nn.Linear(hidden_features , hidden_features, ))
//             self.net.append(nn.ReLU())
        
                
//         self.net.append(nn.Linear(hidden_features, out_features))
//         if not outermost_linear:
//             self.net.append(nn.ReLU())
        
//         self.net = nn.Sequential(*self.net)
    
//     def forward(self, x):
//         return self.net(x)  

// class NeuTex(nn.Module):
//     def __init__(self, resolution, embeddings_ch, bound=10e-4, interp_mode='bilinear'):
//         super().__init__()
//         self.resolution     = resolution
//         self.embeddings_ch  = embeddings_ch
//         self.interp_mode    = interp_mode
//         tex = torch.empty((1, embeddings_ch ,resolution, resolution), dtype=torch.float32, requires_grad=True)
//         torch.nn.init.uniform_(tex, a=-bound, b=bound)
//         self.tex = nn.Parameter(tex)
    
//     def forward(self, uvs):
//         coords = self.uvs2coords(uvs)
//         return self.grid_sample(coords)
    
//     def uvs2coords(self, uvs):
//         coords = uvs - torch.trunc(uvs)
//         return utils.uvs2coords(coords)
    
//     def grid_sample(self, coords):
//         batch_size = coords.shape[0]
//         tex = self.tile_texture(batch_size)
//         return torch.nn.functional.grid_sample(
//             tex, 
//             coords, 
//             mode=self.interp_mode, 
//             padding_mode='zeros', 
//             align_corners=True)
    
//     def tile_texture(self, batch_size):
//         return self.tex.repeat((batch_size,1,1,1))
        
//     def fuse_blur(self, sigma, kernel_size=17):
//         with torch.no_grad():
//             blurred = gaussian_blur(self.tex[0], kernel_size, sigma)
//             blurred = blurred.unsqueeze(0)
//             self.tex.copy_(self.tex * 0.4 + blurred * 0.6)
           
//     def set_interp_mode(self, interp_mode='bilinear'):
//         self.interp_mode = interp_mode

// class NeuMipMap(NeuTex):
//     def __init__(self, resolution,  embeddings_ch, n_levels, bound=10e-4, interp_mode='bilinear', concat=False):
//         super().__init__(resolution,  embeddings_ch, bound=bound, interp_mode=interp_mode)
//         self.concat     = concat # True
//         self.n_levels   = n_levels # 4
//         levels = list()
//         current_resolution = resolution  # 512
//         #TODO could calculate the max level
//         for _ in range(self.n_levels):
//             nex = NeuTex(current_resolution, embeddings_ch, bound=bound)
//             levels.append(nex)
//             current_resolution//=2
//         self.levels = nn.ModuleList(levels)
            
//     def forward(self, uvs, level):
//         coords = self.uvs2coords(uvs)
//         if self.concat:
//             result = list()
//             level = level
//             for n, l in enumerate(self.levels):
//                 sample = l.grid_sample(coords)
//                 sample = sample.permute(0, 2, 3, 1)
//                 z = torch.zeros_like(sample)
//                 y = torch.where(level<=n, sample, z)
//                 result.append(y.permute(0, 3, 1, 2))
//             return torch.cat(result, 1) 
//         else: 
//             if len(self.levels) > 1:
//                 level = level.permute(0, 3, 1, 2)
//                 base = torch.floor(level)
//                 alpha = level - base
//                 alpha = alpha.float()
//                 base = base.long()
//                 base = torch.clamp(base, len(self.levels)-1)
//                 next = base + 1
//                 next = torch.clamp(next, len(self.levels)-1)

//                 base = base.float()
//                 next = next.float()
//                 result = 0.0
//                 for n, l in enumerate(self.levels):
//                     sample = l.grid_sample(coords) 
//                     z = torch.zeros_like(sample[:, :1, :, :])
//                     y = torch.where(base==n, sample, z) *(1. - alpha) + torch.where(next==n, sample, z) * (alpha)

//                     result += y
//                 return result
//             else:
//                 return self.levels[0].grid_sample(coords) 
     
    
//     def fuse_blur(self, sigma, kernel_size=17):
//         with torch.no_grad():
//             for n, l in enumerate(self.levels):
//                 l.fuse_blur(sigma, kernel_size)
//                 kernel_size = kernel_size//2 + 1
    
//     def recreate_pyramid(self):
//         with torch.no_grad():
//             for n in range(self.n_levels):
//                 if n != 0:
//                     tex = self.levels[n-1].tex
//                     tex = torch.nn.functional.interpolate(tex, scale_factor=(.5, .5), mode='area')
//                     tex = tex.detach().requires_grad_()
//                     self.levels[n].tex.copy_(tex)
    
//     def set_interp_mode(self, interp_mode='bilinear'):
//         self.interp_mode = interp_mode
//         for l in self.levels:
//             l.set_interp_mode(self.interp_mode)


// class NeuBTF(nn.Module):
//     def __init__(self, resolution, embeddings_ch, hidden_features, hidden_layers, out_features, outermost_linear=True, 
//                  first_omega_0=30, hidden_omega_0=30., parallax_scale=0.1, concat=False, shared=False, siren=False, n_levels=5):
//         super().__init__()
//         self.concat         = concat  # True
//         self.shared         = shared  # False
//         self.siren          = siren  # True
//         self.resolution     = resolution  # 512
//         self.embeddings_ch  = embeddings_ch # 7
//         self.n_levels       = n_levels # 4
//         self.parallax_scale = parallax_scale
//         if concat:
//             self.embeddings_ch *= n_levels # 28
//             self.tex = NeuMipMap(resolution, embeddings_ch, n_levels, concat=True)
//             if not shared:
//                 self.off_tex = NeuMipMap(resolution, embeddings_ch, n_levels,concat=True)
//         else:
//             self.tex = NeuMipMap(resolution, embeddings_ch, 1)
//             if not shared:
//                 self.off_tex = NeuMipMap(resolution, embeddings_ch, 1)
        
//         self.sigma = 8.0
        
//         net_in_ch = self.embeddings_ch + 6 # 28+6
//         off_net_in_ch = self.embeddings_ch + 3 # 28+3
        
//         if siren:
//             self.net = Siren(
//                 net_in_ch , hidden_features, hidden_layers, 
//                 out_features, outermost_linear=outermost_linear,
//                 first_omega_0=first_omega_0,hidden_omega_0=hidden_omega_0)
//             self.off_net = Siren( 
//                 off_net_in_ch, hidden_features, hidden_layers, 1, 
//                 outermost_linear=outermost_linear,
//                 first_omega_0=first_omega_0,hidden_omega_0=hidden_omega_0)
//         else: 
//             self.net = MLP(
//                 net_in_ch , hidden_features, 
//                 hidden_layers, out_features, 
//                 outermost_linear=outermost_linear)
//             self.off_net = MLP(
//                 off_net_in_ch , hidden_features, 
//                 hidden_layers, 1, 
//                 outermost_linear=outermost_linear)

//     # todo: swap wi, wo with mitsuba    
//     def btf_sample(self, wi, wo, uv, side_len=512) -> np.ndarray:  
//         sample_size = len(wi)
//         print("Shapes:", np.array(uv).shape, np.array(wi).shape, np.array(wo).shape)
//         print("BTF Sampling Size:", sample_size)
//         step = side_len**2
        
//         result = list()

//         for start in range(0, sample_size, step):
//             end = min((start + step, sample_size))
//             s_uv = torch.tensor(uv[start:end], dtype=torch.float32)
//             s_wi = torch.tensor(wi[start:end], dtype=torch.float32)
//             s_wo = torch.tensor(wo[start:end], dtype=torch.float32)

//             print("  Processing samples: ", start, " to ", end)
//             print("    Shapes:", s_uv.shape, s_wi.shape, s_wo.shape)

//             if end == sample_size:
//                 s_uv = s_uv.reshape(( 1, 1, -1, 2))
//                 s_wi = s_wi.reshape((1, 1, -1, 3))
//                 s_wo = s_wo.reshape((1, 1, -1, 3))

//             else: 
//                 s_uv = s_uv.reshape(( 1, side_len, side_len, 2 ))
//                 s_wi = s_wi.reshape((1, side_len, side_len, 3 ))
//                 s_wo = s_wo.reshape((1, side_len, side_len, 3 ))

//             print("    Shapes:", s_uv.shape, s_wi.shape, s_wo.shape)

//             level = torch.zeros_like(s_uv[..., :1])
//             level = level.cuda()
//             s_uv =  s_uv.cuda()
//             s_wi =  s_wi.cuda()
//             s_wo =  s_wo.cuda()

//             y, _, _, _ = self.forward(
//                 s_uv, 
//                 level,
//                 s_wi,
//                 s_wo,
//             )

//             y = y.detach().cpu().reshape((-1,3)).numpy()
//             result.append(y)
//             print("    Result shape:", y.shape)

//         result = np.concatenate(result, axis=0)
//         print("Final result shape:", result.shape)

//         return result
    
//     def forward(self, uvs, level, wi, wo, skip_offset=False):
//         offset, neu_depth = self.calculate_offset(uvs, wo, level)
//         new_uvs = uvs + offset
//         net_in = self.tex(new_uvs, level)
//         net_in = torch.cat(( net_in.permute(0,2,3,1), wi, wo,),-1)
//         result = self.net(net_in)
        
//         return result, net_in[...,:self.embeddings_ch], new_uvs, neu_depth
    
//     def calculate_offset(self, uvs, wo, level):
//         if self.shared:
//             off_in = self.tex(uvs, level)
//         else:
//             off_in = self.off_tex(uvs, level)
//         off_in = torch.cat((off_in.permute(0, 2, 3, 1), wo, ),-1)
        
//         neu_depth = self.off_net(off_in)

//         offset = self.parallax_mapping(neu_depth, wo, self.parallax_scale)
//         offset[...,1] = - offset[...,1]
//         return offset, neu_depth
    
//     def freeze_offset(self):
//         for param in self.off_net.parameters():
//             param.requires_grad = False
//         for param in self.off_tex.parameters():
//             param.requires_grad = False
    
//     def fuse_blur(self, step, max_step):
//         half_step = max_step//2
        
//         if step <  half_step:
//             sigma = self.sigma * (2.**(-step/(half_step//2)))
//             kernel_size = 17 
//             self.tex.fuse_blur(sigma, kernel_size)
//             if not self.shared:
//                 self.off_tex.fuse_blur(sigma, kernel_size)
                
//     def set_interp_mode(self, interp_mode='bilinear'):
//         self.tex.set_interp_mode(interp_mode)
//         if not self.shared:
//             self.off_tex.set_interp_mode(interp_mode)

//     def load(self, path):
//         self.load_state_dict(torch.load(path))
        
//     def parallax_mapping(self, depth, w_o, scale=1.0):
//         p = w_o[..., :2] / torch.clamp(w_o[..., 2:], min=0.6)
//         p *= (depth*scale)
//         return p
    
//     def export_params_as_texture_img(self,path):
//         import os
//         if not os.path.exists(path):
//             os.makedirs(path)

//         def float32_to_rgba8(x:torch.Tensor):
//             # (...,) to (...,4) split bits
//             x_np = x.cpu().numpy()
//             x_np = np.expand_dims(x_np, axis=-1)  # Add a new axis for the last dimension
//             x_bytes = x_np.view(np.uint8)
        
//             return x_bytes

        
//         # NeuMipmap
//         if self.concat:
//             for n, l in enumerate(self.tex.levels):
//                 # l: NeuTex(nn.Module)
//                 params = l.tex.detach().cpu()  # (1, C, H, W)
//                 params = params.squeeze(0)  # (C, H, W)
//                 params = params.permute(1, 2, 0)  # (H, W, C)
//                 img = float32_to_rgba8(params)  # (H, W, C) -> (H, W, C, 4)
//                 for c in range(params.shape[2]):
//                     im_out = img[:,:,c,:] #(H,W,4)
//                     img_path = os.path.join(path, f'tex_level_{n}_channel_{c}.png')
//                     Image.fromarray(im_out,mode="RGBA").save(img_path, 'PNG')
//             if not self.shared:
//                 for n, l in enumerate(self.off_tex.levels):
//                     # l: NeuTex(nn.Module)
//                     params = l.tex.detach().cpu()  # (1, C, H, W)
//                     params = params.squeeze(0)  # (C, H, W)
//                     params = params.permute(1, 2, 0)  # (H, W, C)
//                     img = float32_to_rgba8(params)  # (H, W, C) -> (H, W, C, 4)
//                     for c in range(params.shape[2]):
//                         im_out = img[:,:,c,:] #(H,W,4)
//                         img_path = os.path.join(path, f'offtex_level_{n}_channel_{c}.png')
//                         Image.fromarray(im_out,mode="RGBA").save(img_path, 'PNG')
        
//         # Siren
//         net_params = self.net.state_dict()
//         for name, param in net_params.items():
//             # concatenate weight and bias if exists, e.g., weight: (N,M), bias: (M,) -> (N+1,M)
//             if 'weight' in name:
//                 bias_name = name.replace('weight', 'bias')
//                 if bias_name in net_params:
//                     param = torch.cat([param, net_params[bias_name].unsqueeze(1)], dim=1) # 32 * 35|33
//                 param = param.detach().cpu()
//                 img = float32_to_rgba8(param)  # (N,) -> (N,4)
//                 img_path = os.path.join(path, f'{name}.png')
//                 Image.fromarray(img,mode="RGBA").save(img_path, 'PNG')
//         off_net_params = self.off_net.state_dict()
//         for name, param in off_net_params.items():
//             # concatenate weight and bias if exists, e.g., weight: (N,M), bias: (M,) -> (N+1,M)
//             if 'weight' in name:
//                 bias_name = name.replace('weight', 'bias')
//                 if bias_name in off_net_params:
//                     param = torch.cat([param, off_net_params[bias_name].unsqueeze(1)], dim=1) # 32 * 35|33
//                 param = param.detach().cpu()
//                 img = float32_to_rgba8(param)  # (N,) -> (N,4)
//                 img_path = os.path.join(path, f'off{name}.png')
//                 Image.fromarray(img,mode="RGBA").save(img_path, 'PNG')


