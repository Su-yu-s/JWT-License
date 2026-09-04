package com.example.license.config;

import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;

@Controller
public class PageController {

    @GetMapping("/")
    public void rootRedirect(HttpServletResponse response) throws Exception {
        // 首页重定向到登录页（拦截器会校验未登录）
        response.sendRedirect("/login.html");
    }
}
