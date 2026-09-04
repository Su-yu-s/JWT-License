package com.example.license.config;

import com.example.license.interceptor.AuthInterceptor;
import com.example.license.interceptor.PageAuthInterceptor;
import jakarta.annotation.Resource;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.InterceptorRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

@Configuration
public class WebMvcConfig implements WebMvcConfigurer {

    @Resource
    private AuthInterceptor authInterceptor;

    @Resource
    private PageAuthInterceptor pageAuthInterceptor;

    @Override
    public void addInterceptors(InterceptorRegistry registry) {
        // 页面拦截：未登录重定向到登录页
        registry.addInterceptor(pageAuthInterceptor)
                .addPathPatterns("/*.html");

        // API 拦截：未登录返回 401
        registry.addInterceptor(authInterceptor)
                .addPathPatterns("/api/**")
                .excludePathPatterns(
                        "/api/auth/login",
                        "/api/sdk/activate",
                        "/api/sdk/heartbeat",
                        "/api/sdk/online-check",
                        "/api/sdk/takeover",
                        "/api/sdk/deactivate"
                );
    }
}
