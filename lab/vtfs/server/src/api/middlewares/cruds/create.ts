import { Context, Next } from "koa";
import { repository } from "@models/repository";

export async function create(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const parentIno = parseInt(ctx.query.parent_ino as string, 10) || 0;
    const name = ctx.query.name as string;

    let rawData = ctx.query.data;
    if (Array.isArray(rawData)) {
        rawData = rawData[0] || "";
    }
    const content = Buffer.from(rawData || "", "utf-8");

    console.log("[create] Incoming request:", { token, parentIno, name, dataLength: content.length });

    const newFile = await repository.create(token, parentIno, false, content, name);

    console.log("[create] Response:", { ino: newFile.ino });

    ctx.body = { ino: newFile.ino };
    await next();
}
