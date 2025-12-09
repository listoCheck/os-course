import {Context, Next} from "koa";
import {repository} from "@models/repository";

export async function read(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const ino = parseInt(ctx.query.parent_ino as string, 10) || 0;

    console.log("[read] Incoming request:", { token, ino });

    const file = await repository.findByIno(ino, token);
    if (!file) {
        console.log("[read] File not found");
        ctx.body = "File not found";
        return;
    }

    const sizeBuffer = Buffer.alloc(8);
    sizeBuffer.writeBigInt64LE(BigInt(file.data?.length || 0));
    const responseBuffer = Buffer.concat([sizeBuffer, file.data || Buffer.alloc(0)]);

    console.log("[read] Sending response, total length:", responseBuffer.length);

    ctx.body = responseBuffer;
    await next();
}