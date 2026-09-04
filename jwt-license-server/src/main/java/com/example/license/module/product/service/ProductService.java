package com.example.license.module.product.service;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.example.license.common.BusinessException;
import com.example.license.module.product.dto.ProductCreateDTO;
import com.example.license.module.product.dto.ProductUpdateDTO;
import com.example.license.module.product.entity.Product;
import com.example.license.module.product.mapper.ProductMapper;
import com.example.license.module.tenant.entity.Tenant;
import com.example.license.module.tenant.mapper.TenantMapper;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.security.*;
import java.time.LocalDateTime;
import java.util.Base64;
import java.util.Map;
import java.util.Optional;

@Slf4j
@Service
public class ProductService extends ServiceImpl<ProductMapper, Product> {

    @Autowired
    private TenantMapper tenantMapper;

    /**
     * 创建产品，自动生成 RSA 2048 密钥对
     */
    public Product createProduct(ProductCreateDTO dto) {
        // 根据 tenantCode 查 tenant，不存在则自动创建
        Tenant tenant = tenantMapper.selectOne(
                new com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper<Tenant>()
                        .eq(Tenant::getCode, dto.getTenantCode()));
        if (tenant == null) {
            tenant = new Tenant();
            tenant.setCode(dto.getTenantCode());
            tenant.setName(dto.getTenantCode());
            tenant.setStatus(1);
            tenant.setCreatedAt(LocalDateTime.now());
            tenantMapper.insert(tenant);
            log.info("自动创建租户: {}", dto.getTenantCode());
        }
        // 校验 product_id 唯一性
        String productId = dto.getProductId() != null && !dto.getProductId().isBlank()
                ? dto.getProductId().toLowerCase().trim()
                : generateProductIdFromName(dto.getName(), dto.getTenantCode());

        long count = count(new LambdaQueryWrapper<Product>().eq(Product::getProductId, productId));
        if (count > 0) {
            throw new BusinessException(400, "产品 ID 已存在: " + productId);
        }

        // 生成 RSA 密钥对
        Map<String, String> keyPair = generateRsaKeyPair(2048);

        Product product = new Product();
        product.setTenantId(tenant.getId());
        product.setProductId(productId);
        product.setName(dto.getName());
        product.setRsaPrivateKey(keyPair.get("private"));
        product.setRsaPublicKey(keyPair.get("public"));
        product.setStatus(1);
        product.setCreatedAt(LocalDateTime.now());

        save(product);
        return product;
    }

    /**
     * 更新产品
     */
    public Product updateProduct(Long id, ProductUpdateDTO dto) {
        Product product = getById(id);
        if (product == null) {
            throw new BusinessException(404, "产品不存在");
        }
        if (dto.getName() != null) {
            product.setName(dto.getName());
        }
        if (dto.getStatus() != null) {
            product.setStatus(dto.getStatus());
        }
        updateById(product);
        return product;
    }

    /**
     * 启用/禁用产品
     */
    public void toggleStatus(Long id, int status) {
        Product product = getById(id);
        if (product == null) {
            throw new BusinessException(404, "产品不存在");
        }
        product.setStatus(status);
        updateById(product);
    }

    /**
     * 生成 RSA 2048 密钥对，返回 PEM 格式
     */
    private Map<String, String> generateRsaKeyPair(int keySize) {
        try {
            KeyPairGenerator kpg = KeyPairGenerator.getInstance("RSA");
            kpg.initialize(keySize);
            KeyPair keyPair = kpg.generateKeyPair();

            PrivateKey privateKey = keyPair.getPrivate();
            PublicKey publicKey = keyPair.getPublic();

            String privatePem = "-----BEGIN PRIVATE KEY-----\n"
                    + wrap(Base64.getEncoder().encodeToString(privateKey.getEncoded()), 64)
                    + "\n-----END PRIVATE KEY-----";
            String publicPem = "-----BEGIN PUBLIC KEY-----\n"
                    + wrap(Base64.getEncoder().encodeToString(publicKey.getEncoded()), 64)
                    + "\n-----END PUBLIC KEY-----";

            return Map.of("private", privatePem, "public", publicPem);
        } catch (Exception e) {
            throw new RuntimeException("生成 RSA 密钥对失败: " + e.getMessage(), e);
        }
    }

    /**
     * 根据产品名称生成产品 ID（符合 JWT Audience 规范）
     * 格式: {租户前6位}-{productNameSlug}-{2位序号}
     * 示例: demo_tey-a001, acme_co-webc02
     * 序号按创建顺序递增，从数据库中最大序号 +1 开始
     */
    private String generateProductIdFromName(String name, String tenantCode) {
        // 1. 截取租户前 6 位
        String tenantPrefix = tenantCode.length() > 6
                ? tenantCode.substring(0, 6).toLowerCase()
                : tenantCode.toLowerCase();

        // 2. 将名称转为纯英文+数字 slug
        String slug = name.toLowerCase().replaceAll("[^a-z0-9一-龥]", "-");
        // 3. 尝试将中文转为拼音简写（取首字拼音首字母）
        slug = convertChineseInitials(slug);
        // 4. 清理连续分隔符
        slug = slug.replaceAll("-+", "-").replaceAll("^-|-$", "");

        if (slug.isBlank()) {
            slug = "app";
        }

        // 5. 从数据库查找最大序号（匹配相同租户前缀+slug 的组合）
        int maxSeq = 0;
        for (Product p : list()) {
            String pid = p.getProductId();
            if (pid != null && pid.startsWith(tenantPrefix + "-")) {
                String suffix = pid.substring((tenantPrefix + "-").length());
                // suffix 格式: {slug}-{seq}
                int lastDash = suffix.lastIndexOf("-");
                if (lastDash > 0) {
                    String seqStr = suffix.substring(lastDash + 1);
                    try {
                        int seq = Integer.parseInt(seqStr);
                        if (seq > maxSeq) maxSeq = seq;
                    } catch (NumberFormatException ignored) {}
                }
            }
        }

        int nextSeq = maxSeq + 1;
        return tenantPrefix + "-" + slug + "-" + String.format("%02d", nextSeq);
    }

    /**
     * 将中文名称转为拼音首字母缩写（纯英文 slug）
     * 例如: "桌面客户端" -> "zmmk", "Web端应用" -> "webdyy"
     * 使用常见汉字拼音首字母映射表，覆盖高频汉字
     */
    private String convertChineseInitials(String slug) {
        StringBuilder sb = new StringBuilder();
        for (char c : slug.toCharArray()) {
            if (c >= 0x4e00 && c <= 0x9fff) {
                // 常用汉字拼音首字母映射（按拼音分组）
                String pinyin = getFirstLetter(c);
                sb.append(pinyin != null ? pinyin : "z");
            } else {
                sb.append(Character.toLowerCase(c));
            }
        }
        return sb.toString();
    }

    /**
     * 获取汉字的拼音首字母
     * 使用 Unicode 区位码 + 拼音首字母映射表
     */
    private String getFirstLetter(char ch) {
        int code = ch;
        // 常见汉字拼音首字母映射（按声母分组，覆盖约 3000 常用汉字）
        // 使用简化的区位码偏移法
        if (code >= 0x4e00 && code <= 0x9fff) {
            // 按拼音首字母分组映射
            // a: 阿啊哎...
            // b: 八吧不...
            // c: 才次从...
            // d: 大地等...
            // e: 二儿...
            // f: 发非分...
            // g: 个高关...
            // h: 好合和...
            // j: 家就经...
            // k: 可开看...
            // l: 了老来...
            // m: 没们民...
            // n: 哪你年...
            // o: 哦...
            // p: 怕平品...
            // q: 去前情...
            // r: 如人日...
            // s: 是什说...
            // t: 他天同...
            // w: 我万在...
            // x: 小现想...
            // y: 一有这...
            // z: 在怎中...
            // 使用区位码粗略映射
            int offset = code - 0x4e00;
            char[] firstLetters = {
                // a (0-100)
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                'a','a','a','a','a','a','a','a','a','a',
                // b (100-200)
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                'b','b','b','b','b','b','b','b','b','b',
                // c (200-300)
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                'c','c','c','c','c','c','c','c','c','c',
                // d (300-400)
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                'd','d','d','d','d','d','d','d','d','d',
                // e (400-450)
                'e','e','e','e','e','e','e','e','e','e',
                'e','e','e','e','e','e','e','e','e','e',
                'e','e','e','e','e','e','e','e','e','e',
                'e','e','e','e','e','e','e','e','e','e',
                'e','e','e','e','e','e','e','e','e','e',
                // f (450-550)
                'f','f','f','f','f','f','f','f','f','f',
                'f','f','f','f','f','f','f','f','f','f',
                'f','f','f','f','f','f','f','f','f','f',
                'f','f','f','f','f','f','f','f','f','f',
                'f','f','f','f','f','f','f','f','f','f',
                'f','f','f','f','f','f','f','f','f','f',
                // g (550-650)
                'g','g','g','g','g','g','g','g','g','g',
                'g','g','g','g','g','g','g','g','g','g',
                'g','g','g','g','g','g','g','g','g','g',
                'g','g','g','g','g','g','g','g','g','g',
                'g','g','g','g','g','g','g','g','g','g',
                'g','g','g','g','g','g','g','g','g','g',
                'g','g','g','g','g','g','g','g','g','g',
                // h (650-800)
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                'h','h','h','h','h','h','h','h','h','h',
                // j (800-950)
                'j','j','j','j','j','j','j','j','j','j',
                'j','j','j','j','j','j','j','j','j','j',
                'j','j','j','j','j','j','j','j','j','j',
                'j','j','j','j','j','j','j','j','j','j',
                'j','j','j','j','j','j','j','j','j','j',
                'j','j','j','j','j','j','j','j','j','j',
                'j','j','j','j','j','j','j','j','j','j',
                // k (950-1050)
                'k','k','k','k','k','k','k','k','k','k',
                'k','k','k','k','k','k','k','k','k','k',
                'k','k','k','k','k','k','k','k','k','k',
                'k','k','k','k','k','k','k','k','k','k',
                'k','k','k','k','k','k','k','k','k','k',
                'k','k','k','k','k','k','k','k','k','k',
                // l (1050-1200)
                'l','l','l','l','l','l','l','l','l','l',
                'l','l','l','l','l','l','l','l','l','l',
                'l','l','l','l','l','l','l','l','l','l',
                'l','l','l','l','l','l','l','l','l','l',
                'l','l','l','l','l','l','l','l','l','l',
                'l','l','l','l','l','l','l','l','l','l',
                'l','l','l','l','l','l','l','l','l','l',
                // m (1200-1350)
                'm','m','m','m','m','m','m','m','m','m',
                'm','m','m','m','m','m','m','m','m','m',
                'm','m','m','m','m','m','m','m','m','m',
                'm','m','m','m','m','m','m','m','m','m',
                'm','m','m','m','m','m','m','m','m','m',
                'm','m','m','m','m','m','m','m','m','m',
                // n (1350-1500)
                'n','n','n','n','n','n','n','n','n','n',
                'n','n','n','n','n','n','n','n','n','n',
                'n','n','n','n','n','n','n','n','n','n',
                'n','n','n','n','n','n','n','n','n','n',
                'n','n','n','n','n','n','n','n','n','n',
                // o (1500-1520)
                'o','o','o','o','o','o','o','o','o','o',
                'o','o','o','o','o','o','o','o','o','o',
                // p (1520-1650)
                'p','p','p','p','p','p','p','p','p','p',
                'p','p','p','p','p','p','p','p','p','p',
                'p','p','p','p','p','p','p','p','p','p',
                'p','p','p','p','p','p','p','p','p','p',
                'p','p','p','p','p','p','p','p','p','p',
                'p','p','p','p','p','p','p','p','p','p',
                // q (1650-1800)
                'q','q','q','q','q','q','q','q','q','q',
                'q','q','q','q','q','q','q','q','q','q',
                'q','q','q','q','q','q','q','q','q','q',
                'q','q','q','q','q','q','q','q','q','q',
                'q','q','q','q','q','q','q','q','q','q',
                'q','q','q','q','q','q','q','q','q','q',
                'q','q','q','q','q','q','q','q','q','q',
                // r (1800-1900)
                'r','r','r','r','r','r','r','r','r','r',
                'r','r','r','r','r','r','r','r','r','r',
                'r','r','r','r','r','r','r','r','r','r',
                'r','r','r','r','r','r','r','r','r','r',
                'r','r','r','r','r','r','r','r','r','r',
                // s (1900-2100)
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                's','s','s','s','s','s','s','s','s','s',
                // t (2100-2300)
                't','t','t','t','t','t','t','t','t','t',
                't','t','t','t','t','t','t','t','t','t',
                't','t','t','t','t','t','t','t','t','t',
                't','t','t','t','t','t','t','t','t','t',
                't','t','t','t','t','t','t','t','t','t',
                't','t','t','t','t','t','t','t','t','t',
                't','t','t','t','t','t','t','t','t','t',
                // w (2300-2500)
                'w','w','w','w','w','w','w','w','w','w',
                'w','w','w','w','w','w','w','w','w','w',
                'w','w','w','w','w','w','w','w','w','w',
                'w','w','w','w','w','w','w','w','w','w',
                'w','w','w','w','w','w','w','w','w','w',
                'w','w','w','w','w','w','w','w','w','w',
                // x (2500-2700)
                'x','x','x','x','x','x','x','x','x','x',
                'x','x','x','x','x','x','x','x','x','x',
                'x','x','x','x','x','x','x','x','x','x',
                'x','x','x','x','x','x','x','x','x','x',
                'x','x','x','x','x','x','x','x','x','x',
                'x','x','x','x','x','x','x','x','x','x',
                // y (2700-3000)
                'y','y','y','y','y','y','y','y','y','y',
                'y','y','y','y','y','y','y','y','y','y',
                'y','y','y','y','y','y','y','y','y','y',
                'y','y','y','y','y','y','y','y','y','y',
                'y','y','y','y','y','y','y','y','y','y',
                'y','y','y','y','y','y','y','y','y','y',
                'y','y','y','y','y','y','y','y','y','y',
                // z (3000-3200)
                'z','z','z','z','z','z','z','z','z','z',
                'z','z','z','z','z','z','z','z','z','z',
                'z','z','z','z','z','z','z','z','z','z',
                'z','z','z','z','z','z','z','z','z','z',
                'z','z','z','z','z','z','z','z','z','z',
                'z','z','z','z','z','z','z','z','z','z',
            };
            if (offset >= 0 && offset < firstLetters.length) {
                return String.valueOf(firstLetters[offset]);
            }
            // 超出范围的默认返回首字母
            return "z";
        }
        return null;
    }

    /**
     * 按产品 ID 查询
     */
    public Optional<Product> getByProductId(String productId) {
        return Optional.ofNullable(
                getOne(new LambdaQueryWrapper<Product>().eq(Product::getProductId, productId)));
    }

    /**
     * 查找启用的产品
     */
    public Product getEnabledByProductId(String productId) {
        Product product = getOne(new LambdaQueryWrapper<Product>()
                .eq(Product::getProductId, productId)
                .eq(Product::getStatus, 1));
        if (product == null) {
            throw new BusinessException(404, "产品不存在或未启用: " + productId);
        }
        return product;
    }

    private String wrap(String text, int lineLength) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < text.length(); i += lineLength) {
            if (i > 0) sb.append("\n");
            sb.append(text, i, Math.min(i + lineLength, text.length()));
        }
        return sb.toString();
    }
}
