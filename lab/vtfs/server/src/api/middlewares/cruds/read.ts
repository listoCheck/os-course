import {Context, Next} from "koa";
import {Repository} from "@models/repository";
import {FileLab4} from "@models/file";

export async function read(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const ino = parseInt(ctx.query.parent_ino as string, 10) || 0;
    const file = await Repository.findByIno(ino, token);
    if (!file) {
        ctx.body = "File not found";
        return;
    }
    const sizeBuffer = Buffer.alloc(8);
    sizeBuffer.writeBigInt64LE(BigInt(file.data?.length || 0));
    ctx.body = Buffer.concat([sizeBuffer, file.data || Buffer.alloc(0)]);
    await next();
}