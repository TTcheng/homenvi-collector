# influxDB config

## 1.create user
```sql
CREATE USER "homenvi-collector" WITH PASSWORD 'xxxx';
GRANT WRITE ON homenvi TO "homenvi-collector";
```

## 2.Retention & continuous query
```sql
-- 数据保存策略（Retention Policies）
CREATE RETENTION POLICY "homenvi_rp" ON "homenvi" DURATION 30d REPLICATION 1 DEFAULT
-- 连续查询（continuous queries）
CREATE CONTINUOUS QUERY collections_cq_1h ON homenvi BEGIN SELECT identifier,mean(humidity),mean(celsius),mean(fahrenheit),mean(heatIndexCelsius),mean(heatIndexFahrenheit) INTO collections_1h FROM collections GROUP BY identifier,time(1h) END
```

## 3.verify
- login
```shell
influx -username homenvi-collector -password xxxx
```
- verify: write
```sql
USE homenvi;
INSERT collections,identifier=HomenviCollectorAlpha humidity=66.40,celsius=22.60,fahrenheit=72.68,heatIndexCelsius=22.65,heatIndexFahrenheit=72.77;
```

## 4.add to server
````shell
curl -H "Authorization: Bearer 18467d75-bdbb-43c2-b5f6-c1b102f733f2" -H "Content-Type: application/json" -X POST 'http://localhost/api/homenvi/influxusers' --data '{"username":"homenvi-collector","password":"xxxx", "authorities":"WRITE"}'
````
