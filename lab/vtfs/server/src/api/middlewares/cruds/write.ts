import {Context, Next} from "koa";
import {repository} from "@models/repository";

export async function write(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const ino = parseInt(ctx.query.parent_ino as string, 10) || 0;
    let rawData = ctx.query.data;
    if (Array.isArray(rawData)) {
        rawData = rawData[0] || "";
    }
    const content = Buffer.from(rawData || "", "utf-8");
    const data = { data: content, name: ctx.query.name as string | undefined };
    const updatedFile = await repository.update(ino, token, data);
    ctx.body = { success: !!updatedFile };
    await next();
}