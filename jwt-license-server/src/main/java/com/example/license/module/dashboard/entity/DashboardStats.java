package com.example.license.module.dashboard.entity;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@NoArgsConstructor
@AllArgsConstructor
public class DashboardStats {

    private long tenantCount;
    private long productCount;
    private long keyCount;
    private long activeSessionCount;
}
