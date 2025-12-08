const appConfig = {
    port: Number(process.env.APP_PORT) || 9999,
    DB_HOST: String(process.env.DB_HOST) || "postgres",
    PG_PORT: Number(process.env.DB_PORT) || 9999,
    POSTGRES_USER: String(process.env.DB_USER) || "s408145",
    POSTGRES_PASSWORD: String(process.env.DB_PASS) || "JLzD%6772",
    POSTGRES_DB: String(process.env.DB_NAME) || "studs",
}

export default appConfig;
